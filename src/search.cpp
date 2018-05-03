/*
    Defenchess, a chess engine
    Copyright 2017-2018 Can Cetin, Dogac Eldenk

    Defenchess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Defenchess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Defenchess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "search.h"
#include "move_utils.h"
#include "bitboard.h"
#include "move.h"
#include "movegen.h"
#include "tt.h"
#include "move_utils.h"
#include "see.h"
#include <sys/time.h>
#include <algorithm>
#include "tb.h"

bool main_thread_finished = false;
std::vector<Move> root_moves = {};

void print_pv() {
    int i = 0;
    while (i < main_pv.size) {
        std::cout << move_to_str(main_pv.moves[i]) << " ";
        ++i;
    }
    std::cout << std::endl;
}

void update_main_pv() {
    main_pv.size = pv[0].size;
    for (int i = 0; i < main_pv.size; i++) {
        main_pv.moves[i] = pv[0].moves[i];
    }
}

void set_pv(Move move, int ply) {
    int s = ply + 1;
    pv[ply].moves[ply] = move;
    pv[ply].size = s;

    if (s < MAX_PLY && pv[s].size > s) {
        for (int i = s; i < pv[s].size; i++) {
            pv[ply].moves[i] = pv[s].moves[i];
            ++pv[ply].size;
        }
    }
}

bool is_draw(Position *p) {
    if (p->last_irreversible > 3){
        if (p->last_irreversible > 99)
            return true;

        int repetition_count = 0;
        int index = p->my_thread->search_ply - 2;
        while (index >= p->my_thread->search_ply - p->last_irreversible) {
            index -= 2;
            Position *position = &(p->my_thread->positions[index]);
            if (!position) {
                break;
            }
            if (p->hash == position->hash) {
                if (index >= root_ply)
                    return true;

                ++repetition_count;
                if (repetition_count == 2)
                    return true;
            }
        }
    }
    Material *eval_material = get_material(p);
    if (eval_material->endgame_type == DRAW_ENDGAME) {
        return true;
    }
    return false;
}

void save_killer(Position *p, Move move, int depth, int ply, Move *quiets, int quiets_count) {
    SearchThread *my_thread = p->my_thread;
    if (move != my_thread->killers[ply][0]) {
        my_thread->killers[ply][1] = my_thread->killers[ply][0];
        my_thread->killers[ply][0] = move;
    }
    Piece piece = p->pieces[move_from(move)];
    int bonus = depth > 17 ? 0 : depth * depth;
    my_thread->history[piece][move_to(move)] += bonus;

    for (int i = 0; i < quiets_count; ++i) {
        Move q = quiets[i];
        my_thread->history[p->pieces[move_from(q)]][move_to(q)] -= bonus;
    }

    if ((p-1)->current_move) {
        Square prev_to = move_to((p-1)->current_move);
        Piece prev_piece = p->pieces[prev_to];

        my_thread->counter_moves[prev_piece][prev_to] = move;

        my_thread->countermove_history[piece][move_to(move)] += bonus;
        for (int i = 0; i < quiets_count; ++i) {
            Move q = quiets[i];
            my_thread->countermove_history[p->pieces[move_from(q)]][move_to(q)] -= bonus;
        }
    }
}

bool check_time(Position *p) {
    if (is_main_thread(p)) {
        if (timer_count == 0) {
            gettimeofday(&curr_time, NULL);
            if (time_passed() > myremain) {
                is_timeout = true;
                return true;
            }
        }
        ++timer_count;
    } else if (main_thread_finished) {
        return true;
    }
    return false;
}

int alpha_beta_quiescence(Position *p, int alpha, int beta, int depth, bool in_check) {
    assert(alpha >= -MATE && alpha < beta && beta <= MATE);
    assert(depth <= 0);
    assert(in_check == is_checked(p));

    if (check_time(p)) {
        return TIMEOUT;
    }

    int ply = PLY(p);
    if (is_main_thread(p)) {
        pv[ply].size = 0;
    }
    if (ply >= MAX_PLY) {
        return evaluate(p);
    }

    if (is_draw(p)) {
        return 0;
    }

    bool is_principal = beta - alpha > 1;
    int new_depth = in_check || depth >= 0 ? 0 : -1;

    Move tte_move = 0;
    TTEntry *tte = get_tte(p->hash);
    int tte_score = p->static_eval = UNDEFINED;
    if (tte) {
        tte_move = tte->move;
        if (tte->depth >= new_depth) {
            tte_score = tt_to_score(tte->score, ply);
            if (!is_principal &&
                (tte->flag == FLAG_EXACT ||
                (tte->flag == FLAG_BETA && tte_score >= beta) ||
                (tte->flag == FLAG_ALPHA && tte_score <= alpha))) {
                    return tte_score;
            }
        }
    }

    int best_score;
    if (!in_check) {
        bool is_null = ply > 0 && p->board == (p-1)->board;
        if (is_null) {
            p->static_eval = best_score = tempo * 2 - (p-1)->static_eval;
        } else {
            p->static_eval = best_score = evaluate(p);
        }
        if (best_score >= beta) {
            if (!tte) {
                set_tte(p->hash, 0, new_depth, score_to_tt(best_score, ply), FLAG_BETA);
            }
            return best_score;
        }
        if (is_principal && best_score > alpha) {
            alpha = best_score;
        }
    } else {
        best_score = -INFINITE;
    }

    MoveGen movegen = new_movegen(p, ply, new_depth, tte_move, QUIESCENCE_SEARCH, in_check);

    Move best_move = 0;
    int num_moves = 0;
    while (Move move = next_move(&movegen)) {
        assert(!is_move_empty(move));
        assert(is_pseudolegal(p, move));
        assert(in_check || p->static_eval != UNDEFINED);

        ++num_moves;
        bool checks = gives_check(p, move);
        Piece capture = p->pieces[move_to(move)];
        if (move_type(move) == ENPASSANT)
            capture = white_pawn;

        assert(capture != empty || in_check || checks);

        int delta = p->static_eval + 120;
        if (!in_check && !checks && !is_principal && delta > -KNOWN_WIN && !is_advanced_pawn_push(p, move) && delta + piece_values[capture] <= alpha) {
            continue;
        }

        bool evasion_prunable = in_check &&
                                (depth != 0 || num_moves > 2) &&
                                best_score > MATED_IN_MAX_PLY &&
                                capture == empty;

        if ((!in_check || evasion_prunable) && move_type(move) != PROMOTION && !see_capture(p, move)) {
            continue;
        }

        if (!is_legal(p, move)) {
            --num_moves;
            continue;
        }

        Position *position = make_move(p, move);
        ++p->my_thread->nodes;
        int score = -alpha_beta_quiescence(position, -beta, -alpha, depth - 1, checks);
        undo_move(position);
        assert(is_timeout || main_thread_finished || (score >= -MATE && score <= MATE));

        if (is_timeout)
            return TIMEOUT;

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                if (is_principal && is_main_thread(p)) {
                    set_pv(move, ply);
                }
                best_move = move;
                if (is_principal && score < beta) {
                    alpha = score;
                } else {
                    set_tte(p->hash, move, new_depth, score_to_tt(score, ply), FLAG_BETA);
                    return score;
                }
            }
        }
    }

    if (num_moves == 0 && in_check) {
        return -MATE + ply;
    }

    uint8_t flag = is_principal && best_move ? FLAG_EXACT : FLAG_ALPHA;
    set_tte(p->hash, best_move, new_depth, score_to_tt(best_score, ply), flag);
    assert(best_score >= -MATE && best_score <= MATE);
    return best_score;
}

int alpha_beta(Position *p, int alpha, int beta, int depth, bool in_check, bool cut) {
    assert(-MATE <= alpha && alpha < beta && beta <= MATE);
    assert(0 <= depth);
    assert(in_check == is_checked(p));

    if (check_time(p)) {
        return TIMEOUT;
    }

    int ply = PLY(p);
    if (is_main_thread(p)) {
        pv[ply].size = 0;
    }

    if (ply >= MAX_PLY) {
        return evaluate(p);
    }

    if (is_draw(p)) {
        return 0;
    }

    bool is_principal = beta - alpha > 1;
    bool root_node = is_principal && ply == 0;
    assert(!(is_principal && cut));

    // Mate distance pruning
    if (!root_node) {
        int mate_score = MATE - ply;
        if (mate_score < beta) {
            if (mate_score <= alpha) {
                return alpha;
            }
            beta = mate_score;
        }
    }

    p->current_move = 0;

    Move move;
    Move tte_move = 0;
    int tte_score;
    tte_score = p->static_eval = UNDEFINED;
    TTEntry *tte = get_tte(p->hash);
    if (tte) {
        tte_move = tte->move;
        if (tte->depth >= depth) {
            tte_score = tt_to_score(tte->score, ply);
            if (!is_principal &&
                (tte->flag == FLAG_EXACT ||
                (tte->flag == FLAG_BETA && tte_score >= beta) ||
                (tte->flag == FLAG_ALPHA && tte_score <= alpha))) {
                    if (tte_score >= beta && !in_check && tte_move && !is_capture_or_promotion(p, tte_move)) {
                        save_killer(p, tte_move, depth, ply, nullptr, 0);
                    }
                    return tte_score;
            }
        }
    }

    // Probe tablebase
    if (!root_node && tb_initialized) {
        int wdl = probe_syzygy_wdl(p);
        if (wdl != SYZYGY_FAIL) {
            ++p->my_thread->tb_hits;
            int tb_score = wdl == SYZYGY_LOSS ? MATED_IN_MAX_PLY + ply + 1
                         : wdl == SYZYGY_WIN  ? MATE_IN_MAX_PLY  - ply - 1 : 0;

            uint8_t flag = wdl == SYZYGY_LOSS ? FLAG_ALPHA
                         : wdl == SYZYGY_WIN  ? FLAG_BETA : FLAG_EXACT;

            if (flag == FLAG_EXACT ||
                (flag == FLAG_BETA && tb_score >= beta) ||
                (flag == FLAG_ALPHA && tb_score <= alpha)) {
                    set_tte(p->hash, 0, std::min(depth + 6, MAX_PLY - 1), score_to_tt(tb_score, ply), flag);
                    return tb_score;
                }
        }
    }

    bool is_null = ply > 0 && p->board == (p-1)->board;
    if (!in_check) {
        if (depth < 1) {
            return alpha_beta_quiescence(p, alpha, beta, 0, in_check);
        }
        if (is_null) {
            p->static_eval = tempo * 2 - (p-1)->static_eval;
        } else {
            p->static_eval = evaluate(p);
        }
    }

    if (!in_check && !is_principal) {
        assert(p->static_eval != UNDEFINED);
        // Razoring
        if (depth < 4 && p->static_eval <= alpha - razoring_margin[depth]) {
            if (depth <= 1) {
                return alpha_beta_quiescence(p, alpha, alpha + 1, 0, false);
            }

            int margin = alpha - razoring_margin[depth];
            int quiescence_value = alpha_beta_quiescence(p, margin, margin + 1, 0, false);
            if (quiescence_value <= margin) {
                return quiescence_value;
            }
        }

        // Futility
        if (depth < 7 &&
            p->static_eval - 90 * depth >= beta &&
            p->static_eval < KNOWN_WIN &&
            p->non_pawn_material[p->color]) {
                return p->static_eval;
        }

        // Null move pruning
        if (!is_null && depth > 3 && p->static_eval >= beta && p->non_pawn_material[p->color]) {
            int R = 3 + depth / 4 + std::min((p->static_eval - beta) / PAWN_MID, 3);
            int d = std::max(0, depth - R);

            Position *position = make_null_move(p);
            ++p->my_thread->nodes;
            int null_eval = -alpha_beta(position, -beta, -beta + 1, d, false, !cut);
            undo_move(position);
            if (null_eval >= beta) {
                if (null_eval >= MATE_IN_MAX_PLY)
                    null_eval = beta;

                if (depth < 12 && std::abs(beta) < KNOWN_WIN)
                    return null_eval;

                int verification = alpha_beta(p, beta - 1, beta, d, false, false);

                if (verification >= beta)
                    return beta;
            }
        }

        // Probcut
        if (depth >= 5 && abs(beta) < MATE_IN_MAX_PLY) {
            int rbeta = std::min(beta + 120, MATE_IN_MAX_PLY - 1);
            MoveGen movegen = new_movegen(p, ply, depth, tte_move, NORMAL_SEARCH, in_check);

            while ((move = next_move(&movegen)) != 0) {
                if (is_legal(p, move)) {
                    p->current_move = move;
                    bool checks = gives_check(p, move);

                    Position *position = make_move(p, move);
                    int value = -alpha_beta_quiescence(position, -rbeta, -rbeta + 1, -1, checks);
                    if (value >= rbeta) {
                        value = -alpha_beta(position, -rbeta, -rbeta + 1, depth - 4, checks, !cut);
                    }

                    undo_move(position);
                    if (value >= rbeta) {
                        return value;
                    }
                }
            }
        }
    }

    // Internal iterative deepening
    int new_depth = depth;
    if (!tte_move && depth >= 6 && (is_principal || p->static_eval + 150 >= beta)) {
        new_depth = 3 * depth / 4 - 2;
        alpha_beta(p, alpha, beta, new_depth, in_check, cut);
        tte = get_tte(p->hash);
        if (tte) {
            tte_move = tte->move;
        }
    }

    MoveGen movegen = new_movegen(p, ply, depth, tte_move, NORMAL_SEARCH, in_check);

    Move best_move = 0;
    Move quiets[64];
    int quiets_count = 0;
    int best_score = -INFINITE;
    int num_moves = 0;

    bool improving = false;
    if (ply > 1) {
        improving = p->static_eval >= (p-2)->static_eval || (p-2)->static_eval == UNDEFINED;
    }

    while ((move = next_move(&movegen)) != 0) {
        assert(is_pseudolegal(p, move));
        assert(!is_move_empty(move));
        assert(0 < depth || in_check);

        if (root_node && !std::count(root_moves.begin(), root_moves.end(), move)) {
            continue;
        }

        ++num_moves;

        bool checks = gives_check(p, move);
        bool capture_or_promo = is_capture_or_promotion(p, move);
        bool important = in_check || capture_or_promo || checks || move == tte_move || is_advanced_pawn_push(p, move);

        int extension = 0;
        if (checks && see_capture(p, move)) {
            extension = 1;
        }
        new_depth = depth - 1 + extension;

        if (!root_node && !important && p->non_pawn_material[p->color] && best_score > MATED_IN_MAX_PLY) {
            int reduction = lmr(is_principal, depth, num_moves);
            if (depth < 8 && num_moves >= futility_move_counts[improving][depth]) {
                continue;
            }
            // Reduced depth of the next LMR search
            int lmr_depth = std::max(new_depth - reduction, 0);

            // Futility pruning: parent node
            if (lmr_depth < 7 && p->static_eval + 150 + 120 * lmr_depth <= alpha) {
                continue;
            }
        }

        if (!is_legal(p, move)) {
            --num_moves;
            continue;
        }

        Position *position = make_move(p, move);
        ++p->my_thread->nodes;
        p->current_move = move;
        if (!capture_or_promo && quiets_count < 64) {
            quiets[quiets_count++] = move;
        }

        int score;

        if (is_principal && num_moves == 1) {
            score = -alpha_beta(position, -beta, -alpha, std::max(0, new_depth), checks, false);
        } else {
            // late move reductions
            int reduction = 0;
            if (depth >= 3 && num_moves > 1 && !capture_or_promo) {
                reduction = lmr(is_principal, depth, num_moves);
                if (in_check) {
                    --reduction;
                }
                if (cut) {
                    ++reduction;
                }

                // Piece piece = p->pieces[move_from(move)];
                // int quiet_score = p->my_thread->history[piece][move_to(move)] +
                //                   p->my_thread->countermove_history[piece][move_to(move)];
                reduction = std::max(reduction, 0);
            }

            score = -alpha_beta(position, -alpha - 1, -alpha, std::max(0, new_depth - reduction), checks, true);

            // Verify late move reduction and re-run the search if necessary.
            if (reduction > 0 && score > alpha) {
                score = -alpha_beta(position, -alpha - 1, -alpha, std::max(0, new_depth), checks, !cut);
            }

            if (is_principal && score > alpha && score < beta) {
                score = -alpha_beta(position, -beta, -alpha, std::max(0, new_depth), checks, false);
            }
        }
        undo_move(position);
        assert(is_timeout || main_thread_finished || (score >= -MATE && score <= MATE));

        if (is_timeout)
            return TIMEOUT;

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                if (is_principal && is_main_thread(p)) {
                    set_pv(move, ply);
                }
                best_move = move;
                if (is_principal && score < beta) {
                    alpha = score;
                } else {
                    if (!capture_or_promo) {
                        save_killer(p, move, depth, ply, quiets, quiets_count - 1);
                    }
                    set_tte(p->hash, move, depth, score_to_tt(score, ply), FLAG_BETA);
                    return score;
                }
            }
        }
    }

    if (num_moves == 0) {
        best_score = in_check ? -MATE + ply : 0;
    }

    uint8_t flag = is_principal && best_move ? FLAG_EXACT : FLAG_ALPHA;
    set_tte(p->hash, best_move, depth, score_to_tt(best_score, ply), flag);
    if (!in_check && best_move && !is_capture_or_promotion(p, best_move)) {
        save_killer(p, best_move, depth, ply, quiets, quiets_count - 1);
    }
    assert(best_score >= -MATE && best_score <= MATE);
    return best_score;
}

void think(Position *p) {
    // First check TB
    Move tb_move;
    bool in_check = is_checked(p);

    // Clear root moves
    root_moves.clear();

    int wdl = probe_syzygy_dtz(p, &tb_move);
    if (wdl != SYZYGY_FAIL) {
        // Return draws immediately
        if (wdl == SYZYGY_DRAW) {
            std::cout << "info score cp 0" << std::endl;
            std::cout << "bestmove " << move_to_str(tb_move) << std::endl;
            return;
        }
        root_moves.push_back(tb_move);
    } else {
        Material *eval_material = get_material(p);
        MoveGen movegen = new_movegen(p, 0, 0, 0, NORMAL_SEARCH, in_check);
        while (Move move = next_move(&movegen)) {
            if (is_legal(p, move)) {
                root_moves.push_back(move);            
            }
        }
        if (eval_material->endgame_type == DRAW_ENDGAME || (in_check && root_moves.size() == 1)) {
            std::cout << "bestmove " << move_to_str(movegen.moves[0].move) << std::endl;
            return;
        }
    }
    int previous_guess = -MATE;
    int current_guess = -MATE;
    int init_remain = myremain;

    gettimeofday(&start_ts, NULL);
    int depth = 1;

    std::memset(pv_at_depth, 0, sizeof(pv_at_depth));

    initialize_threads();
    while (depth <= think_depth_limit) {
        int aspiration = 20;
        int alpha = -MATE;
        int beta = MATE;

        if (depth >= 5) {
            alpha = std::max(previous_guess - aspiration, -MATE);
            beta = std::min(previous_guess + aspiration, MATE);
        }

        bool failed_low = false;
        while (true) {
            int score;

            for (int i = 1; i < num_threads; ++i) {
                SearchThread *t = &search_threads[i];
                int thread_depth = depth + (i % 4);
                t->thread_obj = std::thread(alpha_beta, &(t->positions[t->search_ply]), alpha, beta, thread_depth, in_check, false);
            }

            score = alpha_beta(p, alpha, beta, depth, in_check, false);
            main_thread_finished = true;

            // Stop threads
            for (int i = 1; i < num_threads; ++i) {
                SearchThread *t = &search_threads[i];
                t->thread_obj.join();
            }
            main_thread_finished = false;

            if (score > alpha) {
                current_guess = score;
                update_main_pv();
            }
            if (is_timeout) {
                break;
            }
            if (score <= alpha) {
                alpha = std::max(score - aspiration, -MATE);
                failed_low = true;
            } else if (score >= beta) {
                beta = std::min(score + aspiration, MATE);
            } else {
                break;
            }

            aspiration += aspiration / 2 + 5;
            assert(alpha >= -MATE && beta <= MATE);
        }
        if (is_timeout) {
            break;
        }
        if (depth >= 18 && failed_low) {
            myremain = std::min(total_remaining, std::min(init_remain * 4 / 3, myremain * 21 / 20)); // %5 panic time
        }

        gettimeofday(&curr_time, NULL);
        int time_taken = time_passed();
        uint64_t tb_hits = sum_tb_hits();
        std::cout << "info depth " << depth << " seldepth " << main_pv.size << " multipv 1 ";
        std::cout << "tbhits " << tb_hits << " score ";

        if (current_guess <= MATED_IN_MAX_PLY) {
            std::cout << "mate " << ((-MATE - current_guess) / 2 + 1);
        } else if (current_guess >= MATE_IN_MAX_PLY) {
            std::cout << "mate " << ((MATE - current_guess) / 2 + 1);
        } else {
            std::cout << "cp " << current_guess * 100 / PAWN_END;
        }

        uint64_t nodes = sum_nodes();
        std::cout << " nodes " << nodes <<  " nps " << nodes*1000/(time_taken+1) << " time " << time_taken << " pv ";
        print_pv();

        previous_guess = current_guess;
        pv_at_depth[depth - 1] = main_pv.moves[0];

        if (depth >= 18 && depth <= 30 && std::abs(current_guess) < KNOWN_WIN && std::abs(current_guess) > 30 &&
                pv_at_depth[depth - 1] == pv_at_depth[depth - 2] &&
                pv_at_depth[depth - 1] == pv_at_depth[depth - 3] &&
                pv_at_depth[depth - 1] == pv_at_depth[depth - 4] &&
                pv_at_depth[depth - 1] == pv_at_depth[depth - 5] &&
                pv_at_depth[depth - 1] == pv_at_depth[depth - 6]
        ) {
            myremain = std::max(init_remain / 3, myremain * 95 / 100);
        }
        ++depth;
    }
    gettimeofday(&curr_time, NULL);
    std::cout << "bestmove " << move_to_str(main_pv.moves[0]);
    if (main_pv.size > 1) {
        std::cout << " ponder " << move_to_str(main_pv.moves[1]);
    }
    std::cout << std::endl;
    return;
}
