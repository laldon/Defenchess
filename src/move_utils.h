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

#ifndef MOVEUTIL_H
#define MOVEUTIL_H

#include "data.h"
#include "bitboard.h"
#include <iostream>

inline Move _movecast(Square from, Square to, Square type_o) {
    // | from (6) | to (6) | type (4) |
    return (from << 10) | (to << 4) | type_o;
}

std::string move_to_str(Move m);

std::string i2n(int i);
std::string int_to_notation(int a);

void show_position_png(Position *p);

int n2i(char c);

void print_movegen(MoveGen *movegen);

#endif
