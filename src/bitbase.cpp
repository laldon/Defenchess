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

#include "bitbase.h"
#include "data.h"
#include "position.h"
#include "search.h"

using namespace::std;
int kpk_bases[2][48][64][64];
uint64_t bitbase[2*48*64];

const int invalid = 8,
          unknown = 1,
          draw    = 2,
          win     = 4;

int bitbase_index(Color color, int wpawn, int wking, int bking) {
    return color + (wking << 1) + (bking << 7) + ((wpawn - A2) << 13);
}

int find_result(Color c, int wpawn, int wking, int bking) {
    if (c == white && (wpawn + 8 == wking || wpawn + 8 == bking) && !(KING_MASKS[wking] & ~(bfi[wpawn] | KING_MASKS[bking]))) {
        return draw;
    }

    if (c == black && !(KING_MASKS[bking] & ~(PAWN_CAPTURE_MASK[wpawn][white] | KING_MASKS[wking]))) {
        return draw;
    }

    if (c == black && distance(bking, wpawn) == 1 && distance(wking, wpawn) > 1) {
        return draw;
    }

    if (c == white && wpawn + 8 >= A8 && (distance(bking, wpawn + 8) > 1 || distance(wking, wpawn + 8) == 1)) {
        return win;
    }

    Square promotion_square = A8 + col(wpawn);
    int16_t pawn_distance = distance(wpawn, promotion_square);
    if (wpawn <= H2)
        --pawn_distance;

    int16_t black_king_distance = distance(bking, promotion_square);
    if (c == white)
        --pawn_distance;
    else
        --black_king_distance;

    if (black_king_distance >= pawn_distance + 2) {
        return win;
    }

    if (c == white) {
        int outcome = 0;
        Bitboard wking_moves = KING_MASKS[wking] & ~(bfi[wpawn] | KING_MASKS[bking]);
        while (wking_moves) {
            Square index = pop(&wking_moves);
            outcome |= kpk_bases[black][wpawn - A2][index][bking];
        }

        int one_move = wpawn + 8;
        if (one_move <= H7 && one_move != wking && one_move != bking) {
            outcome |= kpk_bases[black][one_move - A2][wking][bking];

            int two_move = one_move + 8;
            if (wpawn <= H2 && two_move != wking && two_move != bking) {
                outcome |= kpk_bases[black][two_move - A2][wking][bking];
            }
        }

        if (outcome & invalid) {
            exit(1);
        }

        if (outcome & win) {
            return win;
        } else if (outcome & unknown) {
            return unknown;
        } else {
            return draw;
        }
    } else {
        int outcome = 0;
        Bitboard bking_moves = KING_MASKS[bking] & ~(PAWN_CAPTURE_MASK[wpawn][white] | KING_MASKS[wking]);
        while (bking_moves) {
            Square index = pop(&bking_moves);
            outcome |= kpk_bases[white][wpawn - A2][wking][index];
        }

        if (outcome & invalid) {
            exit(1);
        }

        if (outcome & draw) {
            return draw;
        } else if (outcome & unknown) {
            return unknown;
        } else {
            return win;
        }
    }

    // Should never be here
    exit(1);
    return unknown;
}

void init_kpk() {
    for (int c = white; c <= black; c++) {
        for (int wpawn = A2; wpawn <= H7; wpawn++) {
            for (int wking = A1; wking <= H8; wking++) {
                for (int bking = A1; bking <= H8; bking++) {
                    if (distance(wking, bking) <= 1) {
                        kpk_bases[c][wpawn - A2][wking][bking] = invalid;
                        continue;
                    }
                    if (wpawn == bking || wpawn == wking) {
                        kpk_bases[c][wpawn - A2][wking][bking] = invalid;
                        continue;
                    }
                    if (c == white && (PAWN_CAPTURE_MASK[wpawn][white] & bfi[bking])) {
                        kpk_bases[c][wpawn - A2][wking][bking] = invalid;
                        continue;
                    }
                    kpk_bases[c][wpawn - A2][wking][bking] = unknown;
                }
            }
        }
    }
    for (int i = 0; i < (2*48*64); i++) {
        bitbase[i] = 0;
    }
}

void generate_bitbase() {
    init_kpk();

    while (true) {
        int count = 0;
        for (int wpawn = H7; wpawn >= A2; wpawn--) {
            for (int wking = H8; wking >= A1; wking--) {
                for (int bking = H8; bking >= A1; bking--) {
                    for (Color c = white; c <= black ; c++) {
                        if (kpk_bases[c][wpawn - A2][wking][bking] != unknown) {
                            continue;
                        }

                        int result = find_result(c, wpawn, wking, bking);
                        if (result != unknown) {
                            ++count;
                        }
                        kpk_bases[c][wpawn - A2][wking][bking] = result;
                        if (result == win) {
                            int idx = bitbase_index(c, wpawn, wking, bking);
                            bitbase[idx / 64] |= bfi[idx & 0x3F];
                        }
                    }
                }
            }
        }
        //std::cout << "updated " << count << std::endl;
        if (count == 0)
            break;
    }
}
