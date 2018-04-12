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
        std::cout << move_to_str_stock(movegen->moves[i].move) << ", ";
    }
    std::cout << std::endl;
}

void score_moves(MoveGen *movegen, ScoreType score_type) {
    if (score_type == SCORE_CAPTURE) {
        for (uint8_t i = movegen->head; i < movegen->tail; ++i) {
            movegen->moves[i].score = score_capture_mvvlva(movegen->position, movegen->moves[i].move);
        }
    } else if (score_type == SCORE_QUIET) {
        for (uint8_t i = movegen->head; i < movegen->tail; ++i) {
            movegen->moves[i].score = score_quiet(movegen->position, movegen->moves[i].move);
        }
    } else { // Evasions
        for (uint8_t i = movegen->head; i < movegen->tail; ++i) {
            if (is_capture(movegen->position, movegen->moves[i].move)) {
                movegen->moves[i].score = score_capture_mvvlva(movegen->position, movegen->moves[i].move);
            } else {
                movegen->moves[i].score = score_quiet(movegen->position, movegen->moves[i].move) - (1 << 30);
            }
        }
    }
}

ScoredMove pick_best(ScoredMove *moves, uint8_t head, uint8_t tail) {
    std::swap(moves[head], *std::max_element(moves + head, moves + tail, scored_move_compare));
    return moves[head];
}

Move next_move(MoveGen *movegen) {
    Move move;
    switch (movegen->stage) {
        case NORMAL_TTE_MOVE:
            ++movegen->stage;
            if (movegen->tte_move) {
                return movegen->tte_move;
            }

        case GOOD_CAPTURES_SORT:
            generate_moves<CAPTURE>(movegen, movegen->position);
            score_moves(movegen, SCORE_CAPTURE);
            ++movegen->stage;

        case GOOD_CAPTURES:
            while (movegen->head < movegen->tail) {
                move = pick_best(movegen->moves, movegen->head++, movegen->tail).move;
                if (see_capture(movegen->position, move) >= 0) {
                    return move;
                }
                movegen->moves[movegen->end_bad_captures++] = ScoredMove{move, 0};
            }
            ++movegen->stage;
            move = movegen->killer_moves[0];
            if (move &&
                move != movegen->tte_move &&
                is_pseudolegal(movegen->position, move) &&
                !is_capture(movegen->position, move)) {
                return move;
            }

        case KILLER_MOVES:
            ++movegen->stage;
            move = movegen->killer_moves[1];
            if (move &&
                move != movegen->tte_move &&
                is_pseudolegal(movegen->position, move) &&
                !is_capture(movegen->position, move)) {
                return move;
            }

        case COUNTER_MOVES:
            ++movegen->stage;
            move = movegen->counter_move;
            if (move &&
                move != movegen->tte_move &&
                move != movegen->killer_moves[0] &&
                move != movegen->killer_moves[1] &&
                is_pseudolegal(movegen->position, move) &&
                !is_capture(movegen->position, move)) {
                return move;
            }

        case QUIETS_SORT:
            movegen->head = movegen->end_bad_captures;
            movegen->tail = movegen->end_bad_captures;
            generate_moves<SILENT>(movegen, movegen->position);
            score_moves(movegen, SCORE_QUIET);
            std::sort(movegen->moves + movegen->head, movegen->moves + movegen->tail, scored_move_compare_greater);
            ++movegen->stage;

        case QUIETS:
            while (movegen->head < movegen->tail) {
                move = movegen->moves[movegen->head++].move;
                if (move != movegen->tte_move &&
                    move != movegen->killer_moves[0] &&
                    move != movegen->killer_moves[1] &&
                    move != movegen->counter_move) {
                    return move;
                }
            }
            ++movegen->stage;
            movegen->head = 0;

        case BAD_CAPTURES:
            if (movegen->head < movegen->end_bad_captures) {
                return movegen->moves[movegen->head++].move;
            }
            break;

        case EVASION_TTE_MOVE:
            ++movegen->stage;
            if (movegen->tte_move) {
                return movegen->tte_move;
            }

        case EVASIONS_SORT:
            generate_evasions(movegen, movegen->position);
            score_moves(movegen, SCORE_EVASION);
            ++movegen->stage;

        case EVASIONS:
            while (movegen->head < movegen->tail) {
                move = pick_best(movegen->moves, movegen->head++, movegen->tail).move;
                if (move != movegen->tte_move) {
                    return move;
                }
            }
            break;

        case QUIESCENCE_TTE_MOVE:
            ++movegen->stage;
            if (movegen->tte_move) {
                return movegen->tte_move;
            }

        case QUIESCENCE_CAPTURES_SORT:
            generate_moves<CAPTURE>(movegen, movegen->position);
            score_moves(movegen, SCORE_CAPTURE);
            ++movegen->stage;

        case QUIESCENCE_CAPTURES:
            while (movegen->head < movegen->tail) {
                move = pick_best(movegen->moves, movegen->head++, movegen->tail).move;
                if (move != movegen->tte_move) {
                    return move;
                }
            }
            break;

        case QUIESCENCE_TTE_MOVE_CHECKS:
            ++movegen->stage;
            if (movegen->tte_move) {
                return movegen->tte_move;
            }

        case QUIESCENCE_CAPTURES_SORT_CHECKS:
            generate_moves<CAPTURE>(movegen, movegen->position);
            score_moves(movegen, SCORE_QUIET);
            ++movegen->stage;

        case QUIESCENCE_CAPTURES_CHECKS:
            if (movegen->head < movegen->tail) {
                return pick_best(movegen->moves, movegen->head++, movegen->tail).move;
            }
            ++movegen->stage;
            movegen->head = 0;
            generate_quiet_checks(movegen, movegen->position);

        case QUIESCENCE_QUIETS_CHECKS:
            while (movegen->head < movegen->tail) {
                move = movegen->moves[movegen->head++].move;
                if (move != movegen->tte_move) {
                    return move;
                }
            }
            break;

        default:
            assert(false);
    }

    return 0;
}

MoveGen new_movegen(Position *p, int ply, int depth, Move tte_move, uint8_t type, bool in_check) {
    Square prev_to = move_to((p-1)->current_move);
    int movegen_stage;
    Move tm;
    if (in_check) {
        tm = tte_move && is_pseudolegal(p, tte_move) ? tte_move : Move(0);
        movegen_stage = EVASION_TTE_MOVE;
    } else {
        if (type == NORMAL_SEARCH) {
            tm = tte_move && is_pseudolegal(p, tte_move) ? tte_move : Move(0);
            movegen_stage = NORMAL_TTE_MOVE;
        } else if (type == QUIESCENCE_SEARCH) {
            assert(depth == 0 || depth == -1);
            tm = tte_move && is_pseudolegal(p, tte_move) && is_capture(p, tte_move) ? tte_move : Move(0);
            if (depth >= 0) {
                movegen_stage = QUIESCENCE_TTE_MOVE_CHECKS;
            } else {
                movegen_stage = QUIESCENCE_TTE_MOVE;
            }
        } else {  // Perft
            tm = tte_move && is_pseudolegal(p, tte_move) ? tte_move : Move(0);
            movegen_stage = NORMAL_TTE_MOVE;
        }
    }

    SearchThread *my_thread = p->my_thread;
    MoveGen movegen = {
        {}, // Moves
        p, // Position
        tm, // tte_move
        {my_thread->killers[ply][0], my_thread->killers[ply][1]}, // killer 2
        (ply > 0) ? my_thread->counter_moves[p->pieces[prev_to]][prev_to] : Move(0), // counter move
        movegen_stage, // stage
        0, // head
        0, // tail
        0, // end bad captures
        ply // ply
    };
    return movegen;
}

void generate_evasions(MoveGen *movegen, Position *p) {
    generate_king_evasions(movegen, p);
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

void generate_king_evasions(MoveGen *movegen, Position *p) {
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

void generate_quiet_checks(MoveGen *movegen, Position *p) {
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
