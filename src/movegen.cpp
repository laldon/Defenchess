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

#include "movegen.h"
#include "bitboard.h"
#include "move.h"
#include "target.h"
#include "see.h"
#include <utility>
#include <algorithm>

bool scored_move_compare(ScoredMove lhs, ScoredMove rhs) { return lhs.score < rhs.score; }
bool scored_move_compare_greater(ScoredMove lhs, ScoredMove rhs) { return lhs.score > rhs.score; }

void print_movegen(MoveGen *movegen) {
    std::cout << "movegen: ";
    for (int i = movegen->head; i < movegen->tail; i++) {
        std::cout << move_to_str(movegen->moves[i]) << ", ";
    }
    std::cout << std::endl;
}

void sort_movegen(MoveGen *movegen) {
    SearchThread *my_thread = movegen->position->my_thread;
    for (int i = movegen->head; i < movegen->tail; i++) {
        Move move = movegen->moves[i];
        if (move == movegen->tte_move) {
            movegen->movegen_tte_move = move;
            continue;
        }
        if (is_capture(movegen->position, move)) {
            int exchange = see_capture(movegen->position, move);
            if (exchange >= 0) {
                movegen->movegen_good_captures[movegen->gcc++] = ScoredMove{move, score_capture_mvvlva(movegen->position, move)};
            } else {
                movegen->movegen_bad_captures[movegen->bcc++] = ScoredMove{move, score_capture_mvvlva(movegen->position, move)};
            }
            continue;
        }
        if (move == my_thread->killers[movegen->ply][0]) {
            movegen->movegen_killer_moves[0] = move;
            continue;
        }
        if (move == my_thread->killers[movegen->ply][1]) {
            movegen->movegen_killer_moves[1] = move;
            continue;
        }
        if (movegen->ply > 0) {
            Square prev_to = move_to((movegen->position - 1)->current_move);
            if (move == my_thread->counter_moves[movegen->position->pieces[prev_to]][prev_to]) {
                movegen->movegen_counter_move = move;
                continue;
            }
        }
        movegen->movegen_quiet_moves[movegen->qc++] = ScoredMove{move, score_quiet(movegen->position, move)};
    }
}

void sort_evasions(MoveGen *movegen) {
    for (int i = movegen->head; i < movegen->tail; i++) {
        Move move = movegen->moves[i];
        if (move == movegen->tte_move) {
            movegen->movegen_tte_move = move;
            continue;
        }
        if (is_capture(movegen->position, move)) {
            movegen->movegen_evasions[movegen->ec++] = ScoredMove{move, score_capture_mvvlva(movegen->position, move)};
            continue;
        }
        movegen->movegen_evasions[movegen->ec++] = ScoredMove{move, score_quiet(movegen->position, move) - (1 << 30)};
        assert(movegen->movegen_evasions[movegen->ec - 1].score < 0);
    }
}

void sort_movegen_quiescence(MoveGen *movegen) {
    for (int i = movegen->head; i < movegen->tail; i++) {
        Move move = movegen->moves[i];
        if (move == movegen->tte_move) {
            movegen->movegen_tte_move = move;
            continue;
        }
        if (is_capture(movegen->position, move)) {
            movegen->movegen_good_captures[movegen->gcc++] = ScoredMove{move, score_capture_mvvlva(movegen->position, move)};
            continue;
        }
        movegen->movegen_quiet_moves[movegen->qc++] = ScoredMove{move, 0};
    }
}

ScoredMove pick_best(ScoredMove *moves, int head, int tail) {
    std::swap(moves[head], *std::max_element(moves + head, moves + tail, scored_move_compare));
    return moves[head];
}

Move next_move(MoveGen *movegen) {
    switch (movegen->stage) {
        case NORMAL_TTE_MOVE:
            ++movegen->stage;
            if (movegen->movegen_tte_move) {
                return movegen->movegen_tte_move;
            }

        case GOOD_CAPTURES_SORT:
            movegen->head = 0;
            ++movegen->stage;

        case GOOD_CAPTURES:
            if (movegen->head < movegen->gcc) {
                return pick_best(movegen->movegen_good_captures, movegen->head++, movegen->gcc).move;
            }
            ++movegen->stage;
            if (movegen->movegen_killer_moves[0]) {
                return movegen->movegen_killer_moves[0];
            }

        case KILLER_MOVES:
            ++movegen->stage;
            if (movegen->movegen_killer_moves[1]) {
                return movegen->movegen_killer_moves[1];
            }

        case COUNTER_MOVES:
            ++movegen->stage;
            if (movegen->movegen_counter_move) {
                return movegen->movegen_counter_move;
            }

        case QUIETS_SORT:
            movegen->head = 0;
            std::sort(movegen->movegen_quiet_moves, movegen->movegen_quiet_moves + movegen->qc, scored_move_compare_greater);
            ++movegen->stage;

        case QUIETS:
            if (movegen->head < movegen->qc) {
                return movegen->movegen_quiet_moves[movegen->head++].move;
            }
            ++movegen->stage;

        case BAD_CAPTURES_SORT:
            movegen->head = 0;
            ++movegen->stage;

        case BAD_CAPTURES:
            if (movegen->head < movegen->bcc) {
                return movegen->movegen_bad_captures[movegen->head++].move;
            }
            break;

        case EVASION_TTE_MOVE:
            ++movegen->stage;
            if (movegen->movegen_tte_move) {
                return movegen->movegen_tte_move;
            }

        case EVASIONS_SORT:
            movegen->head = 0;
            ++movegen->stage;

        case EVASIONS:
            if (movegen->head < movegen->ec) {
                return pick_best(movegen->movegen_evasions, movegen->head++, movegen->ec).move;
            }
            break;

        case QUIESCENCE_TTE_MOVE:
            ++movegen->stage;
            if (movegen->movegen_tte_move) {
                return movegen->movegen_tte_move;
            }

        case QUIESCENCE_CAPTURES_SORT:
            movegen->head = 0;
            ++movegen->stage;

        case QUIESCENCE_CAPTURES:
            if (movegen->head < movegen->gcc) {
                return pick_best(movegen->movegen_good_captures, movegen->head++, movegen->gcc).move;
            }
            ++movegen->stage;
            movegen->head = 0;

        case QUIESCENCE_QUIETS:
            while (movegen->head < movegen->qc) {
                return movegen->movegen_quiet_moves[movegen->head++].move;
            }
            break;

        default:
            assert(false);
    }

    return 0;
}

MoveGen *new_movegen(Position *p, int ply, int depth, Move tte_move, uint8_t type, bool in_check) {
    MoveGen *movegen = &(p->my_thread->movegens[ply]);
    movegen->head = 0;
    movegen->tail = 0;
    movegen->position = p;
    movegen->tte_move = tte_move;
    movegen->movegen_tte_move = 0;
    movegen->ply = ply;

    if (in_check) {
        movegen->ec = 0;
        generate_evasions(movegen);
        sort_evasions(movegen);
        movegen->stage = EVASION_TTE_MOVE;
    } else {
        movegen->gcc = 0;
        movegen->bcc = 0;
        movegen->qc = 0;
        movegen->movegen_killer_moves[0] = 0;
        movegen->movegen_killer_moves[1] = 0;
        movegen->movegen_counter_move = 0;
        if (type == NORMAL_SEARCH) {
            generate_moves<ALL>(movegen);
            sort_movegen(movegen);
            movegen->stage = NORMAL_TTE_MOVE;
        } else if (type == QUIESCENCE_SEARCH) {
            assert(depth == 0 || depth == -1);
            generate_moves<CAPTURE>(movegen);
            if (depth >= 0) {
                generate_quiet_checks(movegen);
            }
            sort_movegen_quiescence(movegen);
            movegen->stage = QUIESCENCE_TTE_MOVE;
        }
    }
    return movegen;
}

void generate_evasions(MoveGen *movegen) {
    generate_king_evasions(movegen);
    Position *p = movegen->position;
    // How many pieces causing check ?
    Bitboard attackers = targeted_from(p, p->board, p->color, p->king_index[p->color]);
    uint8_t piece_count = count(attackers);

    // If 1 : Capture piece causing check or block the way.
    if (piece_count == 1) {
        // Capture or  Block attacker
        Square attacker_index = lsb(attackers);

        // King captures are already handled in generate_king_evasions
        Bitboard capture_attackers = targeted_from(p, p->board, opponent_color(p->color), attacker_index);
        while (capture_attackers) {
            Square index = pop(&capture_attackers);
            if (is_pawn(p->pieces[index])) {
                if (rank(index, p->color) == RANK_7) {
                    Move m = _movecast(index, attacker_index, PROMOTION);
                    append_move(_promoteq(m), movegen);
                    append_move(_promoter(m), movegen);
                    append_move(_promoteb(m), movegen);
                    append_move(_promoten(m), movegen);
                } else {
                    Move m = _movecast(index, attacker_index, NORMAL);
                    append_move(m, movegen);
                }
            } else {
                Move m = _movecast(index, attacker_index, NORMAL);
                append_move(m, movegen);
            }
        }

        // Enpassant capturers
        if (p->enpassant && ENPASSANT_INDEX[p->enpassant] == attacker_index) {
            capture_attackers = targeted_from_enpassant(p, opponent_color(p->color), p->enpassant);
            while (capture_attackers) {
                Square index = pop(&capture_attackers);
                Move m = _movecast(index, p->enpassant, ENPASSANT);
                append_move(m, movegen);
            }
        }

        Bitboard between_two = BETWEEN_MASK[p->king_index[p->color]][attacker_index];

        while (between_two) {
            Square blocking_square = pop(&between_two);
            Bitboard blockers = can_go_to(p, opponent_color(p->color), blocking_square);
            while (blockers) {
                Square index = pop(&blockers);
                if (is_pawn(p->pieces[index]) && rank(index, p->color) == RANK_7) {
                    Move m = _movecast(index, blocking_square, PROMOTION);
                    append_move(_promoteq(m), movegen);
                    append_move(_promoter(m), movegen);
                    append_move(_promoteb(m), movegen);
                    append_move(_promoten(m), movegen);
                } else {
                    Move m = _movecast(index, blocking_square, NORMAL);
                    append_move(m, movegen);
                }
            }
        }
    }
}

void generate_king_evasions(MoveGen *movegen) {
    Position *p = movegen->position;
    Square k_index = p->king_index[p->color];

    // Remove the king from the board temporarily
    p->board ^= bfi[k_index];
    Bitboard b = generate_king_targets(k_index) & ~p->bbs[p->color] & ~color_targets(p, opponent_color(p->color));
    p->board ^= bfi[k_index];
 
    while (b) {
        Square index = pop(&b);
        Move m = _movecast(k_index, index, NORMAL);
        append_evasion(m, movegen);
    }
}

void generate_quiet_checks(MoveGen *movegen) {
    Position *p = movegen->position;
    Square king_index = p->king_index[opponent_color(p->color)];
    Bitboard non_capture = ~p->board;

    //? Possible knight Attack Points
    Bitboard knight_aps = KNIGHT_MASKS[king_index] & non_capture;
    Bitboard knights    = p->bbs[knight(p->color)];
    while (knights) {
        Square pc_index = pop(&knights);
        Bitboard knight_sub_aps = KNIGHT_MASKS[pc_index] & knight_aps;
        while (knight_sub_aps) {
            Move m = _movecast(pc_index, pop(&knight_sub_aps), NORMAL);
            append_move(m, movegen);
        }
    } 

    //? Possible bishop Attack Points
    Bitboard bishop_aps = generate_bishop_targets(p->board, king_index) & non_capture;
    Bitboard bishops    = p->bbs[bishop(p->color)];
    while (bishops) {
        Square pc_index = pop(&bishops);
        Bitboard bishop_sub_aps = generate_bishop_targets(p->board, pc_index) & bishop_aps;
        while (bishop_sub_aps) {
            Move m = _movecast(pc_index, pop(&bishop_sub_aps), NORMAL);
            append_move(m, movegen);
        }
    }


    //? Possible rook Attack Points
    Bitboard rook_aps = generate_rook_targets(p->board, king_index) & non_capture;
    Bitboard rooks    = p->bbs[rook(p->color)];
    while (rooks) {
        Square pc_index = pop(&rooks);
        Bitboard rook_sub_aps = generate_rook_targets(p->board, pc_index) & rook_aps;
        while (rook_sub_aps) {
            Move m = _movecast(pc_index, pop(&rook_sub_aps), NORMAL);
            append_move(m, movegen);
        }
    }


    //? Possible queen Attack Points
    Bitboard queen_aps = rook_aps | bishop_aps;
    Bitboard queens    = p->bbs[queen(p->color)];
    while (queens) {
        Square pc_index = pop(&queens);
        Bitboard queen_sub_aps = generate_queen_targets(p->board, pc_index) & queen_aps;
        while (queen_sub_aps) {
            Move m = _movecast(pc_index, pop(&queen_sub_aps), NORMAL);
            append_move(m, movegen);
        }
    }

    //? Possible discover checks
    Bitboard pinned_pieces = p->pinned[opponent_color(p->color)] & p->bbs[p->color];
    while (pinned_pieces) {
        Square pin_index = pop(&pinned_pieces);
        Piece pin_piece = piece_type(p->pieces[pin_index]);
        Bitboard move_locations = 0;
        switch(pin_piece) {
            case white_knight:
                move_locations |= ~FROMTO_MASK[pin_index][king_index] & generate_knight_targets(pin_index) & non_capture;
                break;
            case white_bishop:
                move_locations |= ~FROMTO_MASK[pin_index][king_index] & generate_bishop_targets(p->board, pin_index) & non_capture;
                break;
            case white_rook:
                move_locations |= ~FROMTO_MASK[pin_index][king_index] & generate_rook_targets(p->board, pin_index) & non_capture;
                break;
            case white_queen:
                move_locations |= ~FROMTO_MASK[pin_index][king_index] & generate_queen_targets(p->board, pin_index) & non_capture;
                break;
            case white_king:
                move_locations |= ~FROMTO_MASK[pin_index][king_index] & generate_king_targets(pin_index) & non_capture;
                break;
            case white_pawn:
                move_locations |= ~FROMTO_MASK[pin_index][king_index] & generate_pawn_targets<SILENT>(p, pin_index) & ~(RANK_1BB | RANK_8BB);
                break;
        }
        while (move_locations) {
            Square move_index = pop(&move_locations);
            Move m = _movecast(pin_index, move_index, NORMAL);
            append_move(m, movegen);
        }
    }

    //? Promotion checks
    Bitboard pawns_promoline = p->bbs[pawn(p->color)] & (p->color == white ? RANK_7BB : RANK_2BB);
    while (pawns_promoline) {
        Square index = pop(&pawns_promoline);
        Bitboard pawn_moves = generate_pawn_targets<SILENT>(p, index);
        while (pawn_moves) {
            Square subindex = pop(&pawn_moves);
            Move m = _movecast(index, subindex, PROMOTION);
            // Only consider queen promos
            if (generate_queen_targets(p->board ^ bfi[index], subindex) & p->bbs[king(opponent_color(p->color))]) {
                append_move(_promoteq(m), movegen);
            }
        }
    }
}
