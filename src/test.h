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

#ifndef TEST_H
#define TEST_H

#include <iostream>
#include <sys/time.h>

#include "bitboard.h"
#include "data.h"
#include "move_utils.h"
#include "move.h"
#include "position.h"
#include "search.h"
#include "movegen.h"
#include "target.h"
#include "magic.h"
#include "see.h"

#include "stdio.h"

uint64_t Perft(int index, Position* p, bool root, bool in_check);

void see_test();
void perft_test();

#endif
