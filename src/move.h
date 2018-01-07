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

#ifndef MOVE_H
#define MOVE_H

#include "data.h"
#include "move_utils.h"
#include "target.h"
#include <cstring>

Position* make_move(Position *p, Move m);
Position* undo_move(Position *p);
Position* make_null_move(Position *p);

bool is_pseudolegal(Position *p, Move m);
bool gives_check(Position *p, Move m);

inline bool is_capture(Position *p, Move move) {
    return p->pieces[move_to(move)] != empty;
}

inline bool is_capture_or_promotion(Position *p, Move move) {
    return p->pieces[move_to(move)] != empty || move_type(move) == PROMOTION;
}

inline bool is_checked(Position *p) {
    Color c = p->color;
    return targeted_from(p, p->board, c, p->king_index[c]);
}

inline bool is_advanced_pawn_push(Position *p, Move move) {
    Square from = move_from(move);
    return is_pawn(p->pieces[from]) && rank(from, p->color) > RANK_4;
}

bool can_king_castle(Position *p);
bool can_queen_castle(Position *p);

inline bool is_checked_opposite(Position *p){
    Color c = p->color ^ 1;
    return targeted_from(p, p->board, c, p->king_index[c]);
}

inline bool is_legal(MoveGen *movegen, Move m) {
    Position *p = movegen->position;
    if (move_type(m) == ENPASSANT) {
        Position *new_p = make_move(p, m);
        bool checked = is_checked_opposite(new_p);
        undo_move(new_p);
        return !checked;
    }

    Square to = move_to(m);
    Square from = move_from(m);
    Piece piece = p->pieces[from];

    if (is_king(piece)) {
        return std::abs(from - to) == 2 || !targeted_from_with_king(p, p->board, p->color, to);
    }

    Bitboard pinned = p->pinned[p->color];
    return pinned == 0 || !on(pinned, from) || (FROMTO_MASK[from][to] & p->bbs[king(p->color)]);
}

inline bool is_position_valid(Position *p) {
    if (distance(p->king_index[white], p->king_index[black]) <= 1)
        return false;

    if (p->color != white && p->color != black)
        return false;

    if (!is_king(p->pieces[p->king_index[white]]) || !is_king(p->pieces[p->king_index[black]]))
        return false;

    if (p->enpassant != 0 && rank(p->enpassant, p->color) != RANK_6)
        return false;

    return true;
}

inline Move _promoteq(Move m) {
    return m | PROMOTION_Q;
}
inline Move _promoter(Move m) {
    return m | PROMOTION_R;
}
inline Move _promoteb(Move m) {
    return m | PROMOTION_B;
}
inline Move _promoten(Move m) {
    return m | PROMOTION_N;
}

inline bool can_king_castle(Position *p) {
    Color curr_c = p->color;
    if (p->castling & can_king_castle_mask[curr_c]) {
        if (!(KING_CASTLE_MASK[curr_c] & p->board)) {
            int sth = lsb(KING_CASTLE_MASK[curr_c]);
            if (!targeted_from_with_king(p, p->board, curr_c, sth-1) &&
                !targeted_from_with_king(p, p->board, curr_c, sth) &&
                !targeted_from_with_king(p, p->board, curr_c, sth+1)) {
                    return true;
                }
        }
    }
    return false;
}

inline bool can_queen_castle(Position *p) {
    Color curr_c = p->color;
    if (p->castling & can_queen_castle_mask[curr_c]) {
        if (!(QUEEN_CASTLE_MASK[curr_c] & p->board)) {
            int sth = lsb(QUEEN_CASTLE_MASK[curr_c]);
            if (!targeted_from_with_king(p, p->board, curr_c, sth+1) &&
                !targeted_from_with_king(p, p->board, curr_c, sth+2) &&
                !targeted_from_with_king(p, p->board, curr_c, sth+3)) {
                    return true;
                }
        }
    }
    return false;
}
#endif
