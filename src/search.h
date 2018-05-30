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

#ifndef SEARCH_H
#define SEARCH_H

#include "data.h"
#include "target.h"
#include "eval.h"
#include <algorithm>
#include <vector>

const int razoring_margin[4] = {0, 333, 353, 324};
const int futility_move_counts[2][8] = {
    {2, 3, 4,  7, 11, 15, 20, 26}, // not improving
    {5, 6, 9, 14, 21, 30, 41, 54}, // improving
};

inline int lmr(bool is_pv, int depth, int num_moves) {
    assert(depth >= 1 && num_moves >= 1);
    return reductions[is_pv][std::min(depth, 63)][std::min(num_moves, 63)];
}

int alpha_beta_quiescence(Position *p, Metadata *md, int alpha, int beta, int depth, bool in_check);
void think(Position *p);
void print_pv();

#endif
