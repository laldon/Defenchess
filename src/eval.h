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

#ifndef EVAL_H
#define EVAL_H

#include "data.h"
#include <sstream>
#include <iomanip>

void print_eval(Position *p);

int evaluate(Position *p);
void init_eval();

inline Color winning_side(Position *p) {
    return p->score.endgame > 0 ? white : black;
}

inline std::string score_str(Score sc){
    std::stringstream stream1, stream2;
    stream1 << std::fixed << std::setprecision(2) << (double) sc.midgame / (double) PAWN_END ;
    stream2 << std::fixed << std::setprecision(2) << (double) sc.endgame / (double) PAWN_END ;
    if (sc.midgame < 0 && sc.endgame < 0)
        return "" + stream1.str() + " :" + stream2.str() + " ";
    if (sc.midgame < 0 && sc.endgame >= 0)
        return "" + stream1.str() + " : " + stream2.str() + " ";
    if (sc.midgame >= 0 && sc.endgame < 0)
        return " " + stream1.str() + " :" + stream2.str() + " ";
    return " " + stream1.str() + " : " + stream2.str() + " ";
}

#endif
