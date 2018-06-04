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

#include "endgame.h"
#include "bitboard.h"
#include <iostream>
#include "bitbase.h"
#include "eval.h"

const int push_to_edges[64] = {
    100, 90, 80, 70, 70, 80, 90, 100,
     90, 70, 60, 50, 50, 60, 70,  90,
     80, 60, 40, 30, 30, 40, 60,  80,
     70, 50, 30, 20, 20, 30, 50,  70,
     70, 50, 30, 20, 20, 30, 50,  70,
     80, 60, 40, 30, 30, 40, 60,  80,
     90, 70, 60, 50, 50, 60, 70,  90,
    100, 90, 80, 70, 70, 80, 90, 100
};
const int push_close[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

int evaluate_kpk(Position* p) {
    Color winner  = p->bbs[pawn(white)] ? white : black;;
    Color loser = opponent_color(winner);

    Square pawn_index = mirror_square(lsb(p->bbs[pawn(winner)]), winner);
    Square wking      = mirror_square(p->king_index[winner], winner);
    Square bking      = mirror_square(p->king_index[loser], winner);
    Color side_to_move = winner == white ? p->color : opponent_color(p->color);
    uint64_t secret_index = side_to_move + (wking << 1) + (bking << 7) + ((pawn_index - A2) << 13);
    if (bitbase[secret_index/64] & bfi[secret_index & 63]) {
        int result = KNOWN_WIN + PAWN_END + row(pawn_index);
        return p->color == winner ? result : -result;
    } else {
        return 0;
    }
}

int evaluate_kxk(Position *p) {
    Color winner = p->score.endgame > 0 ? white : black;
    Color loser = opponent_color(winner);

    Square winner_king = p->king_index[winner];
    Square loser_king = p->king_index[loser];

    int result = p->non_pawn_material[winner] +
                 push_to_edges[loser_king] +
                 push_close[distance(winner_king, loser_king)] +
                 KNOWN_WIN;

    return p->color == winner ? result : -result;
}

