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
#include <mutex>
#include "position.h"

int myremain = 10000;
int total_remaining = 10000;
int moves_to_go = 0;
struct timeval curr_time, start_ts;
volatile bool is_timeout = false;
int think_depth_limit = MAX_PLY;

std::vector<Move> root_moves = {};
int depth_increments[MAX_THREADS] = {
    0, 1, 2, 3, 3, 4, 4, 5,
    5, 5, 6, 6, 6, 6, 7, 7,
    7, 7, 7, 7, 8, 8, 8, 8,
    8, 8, 8, 8, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 10,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11
};

Move pv_at_depth[MAX_PLY * 2];
int  score_at_depth[MAX_PLY * 2];

void print_pv() {
    int i = 0;
    while (i < pv[0].size) {
        std::cout << move_to_str(pv[0].moves[i]) << " ";
        ++i;
    }
    std::cout << std::endl;
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
            if (index < 0) {
                break;
            }
            Position *position = &(p->my_thread->positions[index]);
            if (!position) {
                break;
            }
            if (p->hash == position->hash) {
                if (index >= p->my_thread->root_ply)
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

void update_history(SearchThread *thread, Piece piece, Square to, int bonus) {
    int value = thread->history[piece][to];
    thread->history[piece][to] += bonus - value * std::abs(bonus) / 16384;
}

void save_killer(Position *p, Metadata *md, Move move, int depth, Move *quiets, int quiets_count) {
    SearchThread *my_thread = p->my_thread;
    if (move != md->killers[0]) {
        md->killers[1] = md->killers[0];
        md->killers[0] = move;
    }
    Piece piece = p->pieces[move_from(move)];
    int bonus = depth > 17 ? 0 : depth * depth;
    update_history(my_thread, piece, move_to(move), bonus);

    for (int i = 0; i < quiets_count; ++i) {
        Move q = quiets[i];
        update_history(my_thread, p->pieces[move_from(q)], move_to(q), -bonus);
    }

    if ((md-1)->current_move) {
        Square prev_to = move_to((md-1)->current_move);
        Piece prev_piece = p->pieces[prev_to];

        my_thread->counter_moves[prev_piece][prev_to] = move;
    }
}

bool check_time(Position *p) {
    if (is_main_thread(p)) {
        if (timer_count == 0) {
            gettimeofday(&curr_time, NULL);
            if (time_passed() > total_remaining) {
                is_timeout = true;
                return true;
            }
        }
        ++timer_count;
    }
    return false;
}

int alpha_beta_quiescence(Position *p, Metadata *md, int alpha, int beta, int depth, bool in_check) {
    assert(alpha >= -MATE && alpha < beta && beta <= MATE);
    assert(depth <= 0);
    assert(in_check == is_checked(p));

    int ply = md->ply;
    if (is_main_thread(p)) {
        pv[ply].size = 0;
    }
    if (ply >= MAX_PLY) {
        return evaluate(p);
    }

    if (is_draw(p)) {
        return 0;
    }

    md->current_move = no_move;
    (md+1)->ply = ply + 1;
    bool is_principal = beta - alpha > 1;
    int new_depth = in_check || depth >= 0 ? 0 : -1;

    Move tte_move = no_move;
    bool tt_hit;
    TTEntry *tte = get_tte(p->hash, tt_hit);
    int tte_score;
    tte_score = md->static_eval = UNDEFINED;
    if (tt_hit) {
        tte_move = tte->move;
        if (tte->depth >= new_depth) {
            tte_score = tt_to_score(tte->score, ply);
            if (!is_principal &&
                (tte_flag(tte) == FLAG_EXACT ||
                (tte_flag(tte) == FLAG_BETA && tte_score >= beta) ||
                (tte_flag(tte) == FLAG_ALPHA && tte_score <= alpha))) {
                    return tte_score;
            }
        }
    }

    int best_score;
    if (!in_check) {
        bool is_null = ply > 0 && (md-1)->current_move == null_move;
        if (tt_hit && tte->static_eval != UNDEFINED) {
            md->static_eval = best_score = tte->static_eval;
        } else if (is_null) {
            md->static_eval = best_score = tempo * 2 - (md-1)->static_eval;
        } else {
            md->static_eval = best_score = evaluate(p);
        }
        if (best_score >= beta) {
            if (!tt_hit) {
                set_tte(p->hash, tte, 0, new_depth, score_to_tt(best_score, ply), md->static_eval, FLAG_BETA);
            }
            return best_score;
        }
        if (is_principal && best_score > alpha) {
            alpha = best_score;
        }
    } else {
        best_score = -INFINITE;
    }

    MoveGen movegen = new_movegen(p, md, new_depth, tte_move, QUIESCENCE_SEARCH, in_check);

    Move best_move = no_move;
    int num_moves = 0;
    Move move;
    while ((move = next_move(&movegen)) != no_move) {
        assert(!is_move_empty(move));
        assert(is_pseudolegal(p, move));
        assert(in_check || md->static_eval != UNDEFINED);

        ++num_moves;
        bool checks = gives_check(p, move);
        Piece capture = p->pieces[move_to(move)];
        if (move_type(move) == ENPASSANT)
            capture = white_pawn;

        assert(capture != no_piece || in_check || checks);

        int delta = md->static_eval + 120;
        if (!in_check && !checks && !is_principal && delta > -KNOWN_WIN && !is_advanced_pawn_push(p, move) && delta + piece_values[capture] <= alpha) {
            continue;
        }

        bool evasion_prunable = in_check &&
                                (depth != 0 || num_moves > 2) &&
                                best_score > MATED_IN_MAX_PLY &&
                                capture == no_piece;

        if ((!in_check || evasion_prunable) && move_type(move) != PROMOTION && !see_capture(p, move, 0)) {
            continue;
        }

        if (!is_legal(p, move)) {
            --num_moves;
            continue;
        }

        Position *position = make_move(p, move);
        ++p->my_thread->nodes;
        md->current_move = move;
        int score = -alpha_beta_quiescence(position, md+1, -beta, -alpha, depth - 1, checks);
        undo_move(position);
        assert(is_timeout || (score >= -MATE && score <= MATE));

        int main_thread_depth = search_threads[0].depth.load(std::memory_order_relaxed);
        if (is_timeout && main_thread_depth > 1) {
            return TIMEOUT;
        }

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
                    set_tte(p->hash, tte, move, new_depth, score_to_tt(score, ply), md->static_eval, FLAG_BETA);
                    return score;
                }
            }
        }
    }

    if (num_moves == 0 && in_check) {
        return -MATE + ply;
    }

    uint8_t flag = is_principal && best_move ? FLAG_EXACT : FLAG_ALPHA;
    set_tte(p->hash, tte, best_move, new_depth, score_to_tt(best_score, ply), md->static_eval, flag);
    assert(best_score >= -MATE && best_score <= MATE);
    return best_score;
}

int alpha_beta(Position *p, Metadata *md, int alpha, int beta, int depth, bool in_check, bool cut) {
    assert(-MATE <= alpha && alpha < beta && beta <= MATE);
    assert(in_check == is_checked(p));
    if (depth < 1) {
        return alpha_beta_quiescence(p, md, alpha, beta, 0, in_check);
    }

    int ply = md->ply;
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

    md->current_move = (md+1)->excluded_move = no_move;
    Move excluded_move = md->excluded_move;
    uint64_t pos_hash = p->hash ^ uint64_t(excluded_move << 16);
    (md+2)->killers[0] = (md+2)->killers[1] = no_move;
    (md+1)->ply = ply + 1;

    Move tte_move = no_move;
    int tte_score;
    tte_score = md->static_eval = UNDEFINED;
    bool tt_hit;
    TTEntry *tte = get_tte(pos_hash, tt_hit);
    if (tt_hit) {
        tte_move = tte->move;
        if (tte->depth >= depth) {
            tte_score = tt_to_score(tte->score, ply);
            if (!is_principal &&
                (tte_flag(tte) == FLAG_EXACT ||
                (tte_flag(tte) == FLAG_BETA && tte_score >= beta) ||
                (tte_flag(tte) == FLAG_ALPHA && tte_score <= alpha))) {
                    if (tte_score >= beta && !in_check && tte_move && !is_capture_or_promotion(p, tte_move)) {
                        save_killer(p, md, tte_move, depth, nullptr, 0);
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
                    set_tte(pos_hash, tte, 0, std::min(depth + 6, MAX_PLY - 1), score_to_tt(tb_score, ply), UNDEFINED, flag);
                    return tb_score;
                }
        }
    }

    bool is_null = ply > 0 && (md-1)->current_move == null_move;
    if (!in_check) {
        if (tt_hit && tte->static_eval != UNDEFINED) {
            md->static_eval = tte->static_eval;
        } else if (is_null) {
            md->static_eval = tempo * 2 - (md-1)->static_eval;
        } else {
            md->static_eval = evaluate(p);
        }
    }

    if (!in_check && !is_principal) {
        assert(md->static_eval != UNDEFINED);
        // Razoring
        if (depth < 4 && md->static_eval <= alpha - razoring_margin[depth]) {
            if (depth <= 1) {
                return alpha_beta_quiescence(p, md, alpha, alpha + 1, 0, false);
            }

            int margin = alpha - razoring_margin[depth];
            int quiescence_value = alpha_beta_quiescence(p, md, margin, margin + 1, 0, false);
            if (quiescence_value <= margin) {
                return quiescence_value;
            }
        }

        // Futility
        if (depth < 7 &&
            md->static_eval - 90 * depth >= beta &&
            md->static_eval < KNOWN_WIN &&
            p->non_pawn_material[p->color]) {
                return md->static_eval;
        }

        // Null move pruning
        if (!is_null && depth > 3 && md->static_eval >= beta && p->non_pawn_material[p->color]) {
            int R = 3 + depth / 4 + std::min((md->static_eval - beta) / PAWN_MID, 3);
            int d = std::max(0, depth - R);

            Position *position = make_null_move(p);
            md->current_move = null_move;
            ++p->my_thread->nodes;
            int null_eval = -alpha_beta(position, md+1, -beta, -beta + 1, d, false, !cut);
            undo_move(position);
            if (null_eval >= beta) {
                if (null_eval >= MATE_IN_MAX_PLY)
                    null_eval = beta;

                if (depth < 12 && std::abs(beta) < KNOWN_WIN)
                    return null_eval;

                int verification = alpha_beta(p, md, beta - 1, beta, d, false, false);

                if (verification >= beta)
                    return beta;
            }
        }
    }

    // Internal iterative deepening
    int new_depth = depth;
    if (!tte_move && depth >= 6 && (is_principal || md->static_eval + 150 >= beta)) {
        new_depth = 3 * depth / 4 - 2;
        alpha_beta(p, md, alpha, beta, new_depth, in_check, cut);
        tte = get_tte(pos_hash, tt_hit);
        if (tt_hit) {
            tte_move = tte->move;
            tte_score = tt_to_score(tte->score, ply);
        }
    }

    MoveGen movegen = new_movegen(p, md, depth, tte_move, NORMAL_SEARCH, in_check);

    Move best_move = no_move;
    Move quiets[64];
    int quiets_count = 0;
    int best_score = -INFINITE;
    int num_moves = 0;

    bool improving = false;
    if (ply > 1) {
        improving = md->static_eval >= (md-2)->static_eval || (md-2)->static_eval == UNDEFINED;
    }

    Move move;
    while ((move = next_move(&movegen)) != no_move) {
        assert(is_pseudolegal(p, move));
        assert(!is_move_empty(move));
        assert(0 < depth || in_check);

        if (move == excluded_move) {
            continue;
        }

        if (root_node && !std::count(root_moves.begin(), root_moves.end(), move)) {
            continue;
        }

        ++num_moves;

        bool checks = gives_check(p, move);
        bool capture_or_promo = is_capture_or_promotion(p, move);
        bool important = in_check || capture_or_promo || checks || move == tte_move || is_advanced_pawn_push(p, move);

        int extension = 0;
        if (depth >= 10 &&
            move == tte_move &&
            !root_node &&
            excluded_move == no_move &&
            tte_score != UNDEFINED && std::abs(tte_score) < MATE_IN_MAX_PLY &&
            (tte_flag(tte) == FLAG_EXACT || tte_flag(tte) == FLAG_BETA) &&
            tte->depth >= depth - 3 &&
            is_legal(p, move)) {
                int rbeta = std::max(tte_score - 2 * depth, -MATE + 1);
                md->excluded_move = move;
                int singular_value = alpha_beta(p, md, rbeta - 1, rbeta, depth / 2, in_check, cut);
                md->excluded_move = no_move;

                if (singular_value < rbeta) {
                    extension = 1;
                }
        } else if (checks && see_capture(p, move, 0)) {
            extension = 1;
        }
        new_depth = depth - 1 + extension;

        if (!root_node && !important && p->non_pawn_material[p->color] && best_score > MATED_IN_MAX_PLY) {
            if (depth < 8 && num_moves >= futility_move_counts[improving][depth]) {
                continue;
            }

            int lmr_depth = std::max(new_depth - lmr(is_principal, depth, num_moves), 0);
            if (lmr_depth < 7 && md->static_eval + 150 + 120 * lmr_depth <= alpha) {
                continue;
            }

            if (lmr_depth < 8 && !see_capture(p, move, -20 * lmr_depth * lmr_depth)) {
                continue;
            }
        }

        if (!is_principal && !in_check && best_score > MATED_IN_MAX_PLY && depth < 6 && !see_capture(p, move, -PAWN_END * depth)) {
            continue;
        }

        if (!is_legal(p, move)) {
            --num_moves;
            continue;
        }

        Position *position = make_move(p, move);
        ++p->my_thread->nodes;
        md->current_move = move;
        if (!capture_or_promo && quiets_count < 64) {
            quiets[quiets_count++] = move;
        }

        int score;

        if (is_principal && num_moves == 1) {
            score = -alpha_beta(position, md+1, -beta, -alpha, new_depth, checks, false);
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

                Piece piece = p->pieces[move_from(move)];
                int quiet_score = p->my_thread->history[piece][move_to(move)];
                if (quiet_score <= -8192) {
                    ++reduction;
                }
                reduction = std::max(reduction, 0);
            }

            score = -alpha_beta(position, md+1, -alpha - 1, -alpha, new_depth - reduction, checks, true);

            // Verify late move reduction and re-run the search if necessary.
            if (reduction > 0 && score > alpha) {
                score = -alpha_beta(position, md+1, -alpha - 1, -alpha, new_depth, checks, !cut);
            }

            if (is_principal && score > alpha && score < beta) {
                score = -alpha_beta(position, md+1, -beta, -alpha, new_depth, checks, false);
            }
        }
        undo_move(position);
        assert(is_timeout || (score >= -MATE && score <= MATE));

        int main_thread_depth = search_threads[0].depth.load(std::memory_order_relaxed);
        if ((is_timeout || check_time(p)) && main_thread_depth > 1) {
            return TIMEOUT;
        }

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
                        save_killer(p, md, move, depth, quiets, quiets_count - 1);
                    }
                    if (excluded_move == no_move) {
                        set_tte(pos_hash, tte, move, depth, score_to_tt(score, ply), md->static_eval, FLAG_BETA);
                    }
                    return score;
                }
            }
        }
    }

    if (num_moves == 0) {
        best_score = excluded_move != no_move ? alpha : in_check ? -MATE + ply : 0;
    }

    if (excluded_move == no_move) {
        uint8_t flag = is_principal && best_move ? FLAG_EXACT : FLAG_ALPHA;
        set_tte(pos_hash, tte, best_move, depth, score_to_tt(best_score, ply), md->static_eval, flag);
    }
    if (!in_check && best_move && !is_capture_or_promotion(p, best_move)) {
        save_killer(p, md, best_move, depth, quiets, quiets_count - 1);
    }
    assert(best_score >= -MATE && best_score <= MATE);
    return best_score;
}

void thread_think(SearchThread *my_thread, bool in_check) {
    Position *p = &my_thread->positions[my_thread->search_ply];
    Metadata *md = &my_thread->metadatas[0];
    bool is_main = is_main_thread(p);

    int previous_guess = -MATE;
    int current_guess = -MATE;
    int init_remain = myremain;
    int depth = 0;

    while (++depth <= think_depth_limit) {
        if (!is_main) {
            int main_thread_depth = search_threads[0].depth.load(std::memory_order_relaxed);
            depth = main_thread_depth + depth_increments[my_thread->thread_id];
        }

        my_thread->depth = depth;

        int aspiration = 10;
        int alpha = -MATE;
        int beta = MATE;

        if (depth >= 5) {
            alpha = std::max(previous_guess - aspiration, -MATE);
            beta = std::min(previous_guess + aspiration, MATE);
        }

        bool failed_low = false;
        while (true) {
            int score = alpha_beta(p, md, alpha, beta, depth, in_check, false);

            current_guess = score;

            if (is_timeout) {
                break;
            }

            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(score - aspiration, -MATE);
                failed_low = true;
            } else if (score >= beta) {
                beta = std::min(score + aspiration, MATE);
            } else {
                break;
            }

            aspiration += aspiration / 2;
            assert(alpha >= -MATE && beta <= MATE);
        }

        previous_guess = current_guess;

        if (is_timeout) {
            break;
        }

        if (!is_main) {
            continue;
        }

        gettimeofday(&curr_time, NULL);
        int time_taken = time_passed();
        uint64_t tb_hits = sum_tb_hits();
        std::cout << "info depth " << depth << " seldepth " << pv[0].size << " multipv 1 ";
        std::cout << "tbhits " << tb_hits << " score ";

        if (current_guess <= MATED_IN_MAX_PLY) {
            std::cout << "mate " << ((-MATE - current_guess) / 2 + 1);
        } else if (current_guess >= MATE_IN_MAX_PLY) {
            std::cout << "mate " << ((MATE - current_guess) / 2 + 1);
        } else {
            std::cout << "cp " << current_guess * 100 / PAWN_END;
        }

        std::cout << " hashfull " << hashfull();

        uint64_t nodes = sum_nodes();
        std::cout << " nodes " << nodes <<  " nps " << nodes*1000/(time_taken+1) << " time " << time_taken << " pv ";
        print_pv();

        if (time_passed() > myremain) {
            is_timeout = true;
            break;
        }

        pv_at_depth[depth - 1] = pv[0].moves[0];
        score_at_depth[depth - 1] = current_guess;

        if (depth >= 6) {
            if (depth >= 10 && failed_low) {
                myremain = myremain * 11 / 10;
            }
            int score_diff = score_at_depth[depth - 1] - score_at_depth[depth - 2];

            if (score_diff < -10) {
                myremain = myremain * 21 / 20;
            }
            if (score_diff > 10) {
                myremain = std::max(init_remain / 2, myremain * 98 / 100);
            }
            if (pv_at_depth[depth - 1] == pv_at_depth[depth - 2]) {
                myremain = std::max(init_remain / 2, myremain * 94 / 100);
            } else {
                myremain = std::max(init_remain, myremain);
            }
        }
    }
}

void think(Position *p, std::vector<std::string> word_list) {
    init_time(p, word_list);

    // Set the table generation
    start_search();

    // First check TB
    Move tb_move;
    bool in_check = is_checked(p);
    SearchThread *main_thread = p->my_thread;
    Metadata *md = &main_thread->metadatas[0];

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
        MoveGen movegen = new_movegen(p, md, 0, no_move, NORMAL_SEARCH, in_check);
        Move move;
        while ((move = next_move(&movegen)) != no_move) {
            if (is_legal(p, move)) {
                root_moves.push_back(move);
            }
        }
        if (eval_material->endgame_type == DRAW_ENDGAME || root_moves.size() == 1) {
            std::cout << "bestmove " << move_to_str(root_moves[0]) << std::endl;
            return;
        }
    }

    gettimeofday(&start_ts, NULL);
    std::memset(pv_at_depth, 0, sizeof(pv_at_depth));
    std::memset(score_at_depth, 0, sizeof(score_at_depth));

    initialize_threads();
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->thread_obj = std::thread(thread_think, t, in_check);
    }

    // Stop threads
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->thread_obj.join();
    }

    gettimeofday(&curr_time, NULL);
    std::cout << "info time " << time_passed() << std::endl;
    std::cout << "bestmove " << move_to_str(pv[0].moves[0]);
    if (pv[0].size > 1) {
        std::cout << " ponder " << move_to_str(pv[0].moves[1]);
    }
    std::cout << std::endl;
}

void bench() {
    uint64_t nodes = 0;
    std::vector<std::string> empty_word_list;

    struct timeval bench_start, bench_end;
    gettimeofday(&bench_start, nullptr);
    int tmp_depth = think_depth_limit;
    int tmp_myremain = myremain;
    think_depth_limit = 13;
    is_timeout = false;

    for (int i = 0; i < 36; i++){
        std::cout << "\nPosition [" << (i + 1) << "|36]\n" << std::endl;
        Position *p = import_fen(benchmarks[i], 0);

        myremain = 3600000;
        think(p, empty_word_list);
        nodes += search_threads[0].nodes;

        clear_tt();
    }

    gettimeofday(&bench_end, nullptr);
    int time_taken = bench_time(bench_start, bench_end);
    think_depth_limit = tmp_depth;
    myremain = tmp_myremain;

    std::cout << "\n------------------------\n";
    std::cout << "Time  : " << time_taken << std::endl;
    std::cout << "Nodes : " << nodes << std::endl;
    std::cout << "NPS   : " << nodes * 1000 / (time_taken + 1) << std::endl;
    exit(EXIT_SUCCESS);
}

