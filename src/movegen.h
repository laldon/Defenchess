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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "data.h"
#include "bitboard.h"
#include "move.h"
#include "target.h"
#include <cstring>

MoveGen new_movegen(Position *p, Metadata *metadata, int ply, int depth, Move tt_move, uint8_t type, bool in_check);

void generate_evasions(MoveGen *movegen, Position *p);
void generate_king_evasions(MoveGen *movegen, Position *p);
Move next_move(MoveGen *movegen);

inline int score_quiet(Position *p, Move move) {
    Piece piece = p->pieces[move_from(move)];
    return p->my_thread->history[piece][move_to(move)] +
           p->my_thread->countermove_history[piece][move_to(move)];
}

inline int score_capture_mvvlva(Position *p, Move move) {
    Piece from_piece = p->pieces[move_from(move)];
    Piece to_piece = p->pieces[move_to(move)];

    // For a pawn capturing a queen, we get 1200 - 2
    // For a queen capturing a pawn, we get 100 - 10 or something
    return mvvlva_values[to_piece][from_piece];
}

inline bool no_moves(MoveGen *movegen) {
    return movegen->tail == movegen->head;
}

inline void append_move(Move m, MoveGen *movegen) {
#ifdef __PERFT__
    if (is_legal(movegen->position, m)){
        movegen->moves[movegen->tail] = ScoredMove{m, UNDEFINED};
        ++movegen->tail;
    }
#else
    movegen->moves[movegen->tail] = ScoredMove{m, UNDEFINED};
    ++movegen->tail;
#endif
}

// Should not have to check legality...
inline void append_evasion(Move m, MoveGen *movegen) {
    movegen->moves[movegen->tail] = ScoredMove{m, UNDEFINED};
    ++movegen->tail;
}

template<MoveGenType Type> inline Bitboard type_mask(Position *p) {
    if (Type == SILENT) {
        return ~p->board;
    } else if (Type == CAPTURE) {
        return p->bbs[opponent_color(p->color)];
    } else {
        return ~p->bbs[p->color];
    }
}

template<MoveGenType Type>
void generate_piece_moves(MoveGen *movegen, Position *p) {
    Bitboard mask = type_mask<Type>(p);

    Bitboard bbs = p->bbs[knight(p->color)];
    while (bbs) {
        Square outpost = pop(&bbs);
        Bitboard b = generate_knight_targets(outpost) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(outpost, index, NORMAL);
            append_move(m, movegen);
        }
    }
    
    bbs = p->bbs[bishop(p->color)];
    while (bbs) {
        Square outpost = pop(&bbs);
        Bitboard b = generate_bishop_targets(p->board, outpost) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(outpost, index, NORMAL);
            append_move(m, movegen);
        }
    }

    bbs = p->bbs[rook(p->color)];
    while (bbs) {
        Square outpost = pop(&bbs);
        Bitboard b = generate_rook_targets(p->board, outpost) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(outpost, index, NORMAL);
            append_move(m, movegen);
        }
    }

    bbs = p->bbs[queen(p->color)];
    while (bbs) {
        Square outpost = pop(&bbs);
        Bitboard b = generate_queen_targets(p->board, outpost) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(outpost, index, NORMAL);
            append_move(m, movegen);
        }
    }
}

template<MoveGenType Type>
void generate_pawn_moves(MoveGen *movegen, Position *p) {
    Color curr_c = p->color;
    Bitboard bbs = p->bbs[pawn(curr_c)];
    while (bbs) {
        Square outpost = pop(&bbs);
        Bitboard b = generate_pawn_targets<Type>(p, outpost);
        while (b) {
            Square index = pop(&b);
            int r = rank(index, curr_c);
            if (r == RANK_8) {
                Move m = _movecast(outpost, index, PROMOTION);
                append_move(_promoteb(m), movegen);
                append_move(_promoter(m), movegen);
                append_move(_promoten(m), movegen);
                append_move(_promoteq(m), movegen);
            } else if (p->enpassant && index == p->enpassant) {
                Move m = _movecast(outpost, index, ENPASSANT);
                append_move(m, movegen);
            } else {
                Move m = _movecast(outpost, index, NORMAL);
                append_move(m, movegen);
            }
        }
    }
}

template<MoveGenType Type>
void generate_king_moves(MoveGen *movegen, Position *p) {
    Square outpost = p->king_index[p->color];
    Bitboard b = 0;
    if (Type != CAPTURE) {
        if (can_king_castle(p)) {
            b |= bfi_king_castle[p->color];
        }
        if (can_queen_castle(p)) {
            b |= bfi_queen_castle[p->color];
        }
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(outpost, index, CASTLING);
            append_move(m, movegen);
        }
    }

    Bitboard mask = type_mask<Type>(p);
    b = generate_king_targets(outpost) & mask;
    while (b) {
        Square index = pop(&b);
        Move m = _movecast(outpost, index, NORMAL);
        append_move(m, movegen);
    }
}

template<MoveGenType Type>
void generate_moves(MoveGen *movegen, Position *p) {
    generate_pawn_moves<Type>(movegen, p);
    generate_piece_moves<Type>(movegen, p);
    generate_king_moves<Type>(movegen, p);
}

void generate_quiet_checks(MoveGen *movegen, Position *p);
#endif
