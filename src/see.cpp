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

#include "data.h"
#include "target.h"
#include "see.h"
#include "bitboard.h"
#include <iostream>
#include "move_utils.h"

int get_smallest_attacker(Position *p, Bitboard board, Color color, Square square) {
    Bitboard targeters = targeted_from_with_king(p, board, color, square) & board;
    int piece_val_c = piece_values[white_king];
    int curr_index = -1;
    while (targeters) {
        Square index = pop(&targeters);
        if (piece_values[p->pieces[index]] < piece_val_c) {
            piece_val_c = piece_values[p->pieces[index]];
            curr_index = index;
        }
    }

    return curr_index;
}

int see(Position *p, Bitboard board, Color color, int piece_value, Square to) {
    int val = 0;
    int smallest_attacker = get_smallest_attacker(p, board, color, to);

    if (smallest_attacker >= 0) {
        board ^= bfi[smallest_attacker];
        val = piece_value - see(p, board, opponent_color(color), piece_values[p->pieces[smallest_attacker]], to);
    }
    return val;
}

int see_capture(Position *p, Move move) {
    Move m_type = move_type(move);
    if (m_type == ENPASSANT) {
        return PAWN_MID;
    }

    Bitboard board = p->board;
    Square from = move_from(move);
    Square to = move_to(move);
    
    int piece_value = piece_values[p->pieces[to]];
    int capturer_value = piece_values[p->pieces[from]];

    if (is_promotion(move_type(move))) {
        piece_value = piece_values[promotion_piece(move, white)] - piece_values[white_pawn];
    }

    board ^= bfi[from];
    return piece_value - see(p, board, piece_color(p->pieces[from]), capturer_value, to);
}

