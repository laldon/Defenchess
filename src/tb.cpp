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

#include "tb.h"
#include "fathom/src/tbprobe.cpp"
#include <iostream>

void init() {
    const std::string syzygy_path = "/Users/can/Downloads/syzygy";
    tb_init(syzygy_path.c_str());
}

unsigned probe(Position *p) {
    return tb_probe_wdl(
        uint64_t(p->bbs[white]),
        uint64_t(p->bbs[black]),
        uint64_t(p->bbs[king(white)] | p->bbs[king(black)]),
        uint64_t(p->bbs[queen(white)] | p->bbs[queen(black)]),
        uint64_t(p->bbs[rook(white)] | p->bbs[rook(black)]),
        uint64_t(p->bbs[bishop(white)] | p->bbs[bishop(black)]),
        uint64_t(p->bbs[knight(white)] | p->bbs[knight(black)]),
        uint64_t(p->bbs[pawn(white)] | p->bbs[pawn(black)]),
        unsigned(p->last_irreversible),
        unsigned(p->castling),
        unsigned(p->enpassant),
        bool(p->color)
    );
}
