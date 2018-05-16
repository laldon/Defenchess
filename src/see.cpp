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

int get_smallest_attacker(Position *p, Bitboard targeters, Color color) {
    Piece piece = color == white ? white_pawn : black_pawn;
    Piece target = color == white ? white_queen : black_queen;

    for (; piece <= target; piece += 2) {
        Bitboard intersection = p->bbs[piece] & targeters;
        if (intersection) {
            return (int) lsb(intersection);
        }
    }

    return -1;
}

bool see_capture(Position *p, Move move) {
    Move m_type = move_type(move);
    if (m_type != NORMAL) {
        return true;
    }

    Square from = move_from(move);
    Square to = move_to(move);
    Color color = piece_color(p->pieces[from]);
    
    int piece_value = piece_values[p->pieces[to]];
    int capturer_value = piece_values[p->pieces[from]];

    int balance = piece_value - capturer_value;
    if (balance >= 0) {
        return true;
    }

    bool opponent_to_move = true;

    Bitboard board = p->board ^ bfi[from];
    Bitboard targeters = all_targets(p, board, to);

    Bitboard rooks = p->bbs[white_rook] | p->bbs[black_rook];
    Bitboard bishops = p->bbs[white_bishop] | p->bbs[black_bishop];
    Bitboard queens = p->bbs[white_queen] | p->bbs[black_queen];

    while (true) {
        int smallest_attacker = get_smallest_attacker(p, targeters & board & p->bbs[opponent_color(color)], opponent_color(color));
        if (smallest_attacker == -1) {
            break;
        }

        board ^= bfi[smallest_attacker];
        int p_type = piece_type(p->pieces[smallest_attacker]);
        if (p_type == PAWN || p_type == BISHOP || p_type == QUEEN) {
            targeters |= generate_bishop_targets(board, to) & (bishops | queens);
        }

        if (p_type == ROOK || p_type == QUEEN) {
            targeters |= generate_rook_targets(board, to) & (rooks | queens);
        }

        balance += piece_values[p->pieces[smallest_attacker]];;

        opponent_to_move = !opponent_to_move;

        if (balance < 0) {
            break;
        }

        balance = - balance - 1;
        color = opponent_color(color);
    }
    return opponent_to_move;
}
