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

#include "pst.h"

Score pst[14][64];

int bonusPawn[2][64] = {
    {
        0,    0,    0,    0,
       -2,   10,   -4,   -2,
       -4,   -4,   -3,    0,
       -4,    2,    1,   12,
      -10,    0,   12,   20,
      -10,    0,   10,   14,
       -7,    4,    4,    0,
        0,    0,    0,    0
    }, {
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0
    }
};

int bonusKnight[2][64] = {
    {
     -114,  -33,  -21,  -15,
      -32,  -10,    3,    9,
       -5,   18,   30,   36,
      -15,    8,   20,   26,
      -14,    9,   21,   27,
      -41,  -12,    0,    5,
      -47,  -25,  -12,   -6,
      -95,  -56,  -47,  -42
    }, {
      -64,  -42,  -26,   -8,
      -34,  -27,  -11,    7,
      -27,  -19,   -3,   15,
      -21,  -14,    3,   20,
      -21,  -14,    3,   20,
      -27,  -19,   -3,   15,
      -34,  -27,  -11,    7,
      -62,  -42,  -26,   -8
    }
};

int bonusBishop[2][64] = {
    {
      -20,   -8,  -15,  -17,
      -14,   10,   -3,   -1,
      -10,    9,    7,    1,
       -6,   16,    9,    5,
       -6,   17,   12,    6,
       -5,   16,   12,    6,
      -12,   12,    7,    0,
      -27,   -7,  -14,  -20
    }, {
      -33,  -18,  -22,  -13,
      -22,  -10,  -11,   -2,
      -17,   -5,   -6,    3,
      -18,   -6,   -7,    2,
      -18,   -6,   -7,    2,
      -17,   -5,   -6,    3,
      -22,  -10,  -11,   -2,
      -33,  -21,  -22,  -13
    }
};

int bonusRook[2][64] = {
    {
      -13,   -9,   -6,   -4,
       -6,    2,    5,    7,
      -12,   -4,    0,    1,
      -13,   -4,    0,    1,
      -13,   -4,    0,    1,
      -12,   -4,   -2,    1,
      -12,   -4,   -1,    0,
      -15,   -9,   -9,   -5
    }, {
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0,
        0,    0,    0,    0
    }
};

int bonusQueen[2][64] = {
    {
       -1,   -1,   -1,   -1,
       -1,    4,    4,    4,
       -1,    4,    4,    4,
       -1,    4,    4,    4,
       -1,    4,    4,    4,
       -1,    4,    4,    4,
       -1,    4,    4,    4,
       -1,   -1,   -1,   -1
    }, {
      -40,  -27,  -21,  -15,
      -27,  -15,   -9,   -3,
      -21,   -9,   -3,    3,
      -15,   -3,    3,    9,
      -15,   -3,    3,    9,
      -21,   -9,   -3,    3,
      -27,  -15,   -9,   -3,
      -40,  -27,  -21,  -15
    }
};

int bonusKing[2][64] = {
    {
       49,   67,   37,   13,
       60,   77,   47,   23,
       74,   91,   61,   37,
       87,  105,   75,   51,
       99,  116,   86,   62,
      113,  130,  101,   65,
      155,  175,  138,  105,
      157,  188,  158,  114
    }, {
       14,   41,   54,   58,
       37,   64,   78,   82,
       56,   83,   97,  101,
       68,   95,  109,  113,
       68,   95,  109,  113,
       56,   83,   97,  101,
       37,   64,   78,   82,
       14,   41,   54,   58
    }
};

void init_values() {
    for (int s = A1; s <= H8; s++) {
        int flip = s ^ A8;

        int flip_row = row(flip);
        int flip_col = col(flip);
        if (flip_col < 4) {
            flip = flip_row * 4 + flip_col;
        } else {
            flip = flip_row * 4 + 7 - flip_col;
        }

        int sq = s;
        int sq_row = row(sq);
        int sq_col = col(sq);
        if (sq_col < 4) {
            sq = sq_row * 4 + sq_col;
        } else {
            sq = sq_row * 4 + 7 - sq_col;
        }

        pst[white_pawn][s]   = Score{bonusPawn  [0][flip], bonusPawn  [1][flip]} + Score{PAWN_MID, PAWN_END};
        pst[white_knight][s] = Score{bonusKnight[0][flip], bonusKnight[1][flip]} + Score{KNIGHT_MID, KNIGHT_END};
        pst[white_bishop][s] = Score{bonusBishop[0][flip], bonusBishop[1][flip]} + Score{BISHOP_MID, BISHOP_END};
        pst[white_rook][s]   = Score{bonusRook  [0][flip], bonusRook  [1][flip]} + Score{ROOK_MID, ROOK_END};
        pst[white_queen][s]  = Score{bonusQueen [0][flip], bonusQueen [1][flip]} + Score{QUEEN_MID, QUEEN_END};
        pst[white_king][s]   = Score{bonusKing  [0][flip], bonusKing  [1][flip]} + Score{0, 0};

        pst[black_pawn][s]   = Score{-bonusPawn  [0][sq], -bonusPawn  [1][sq]} - Score{PAWN_MID, PAWN_END};
        pst[black_knight][s] = Score{-bonusKnight[0][sq], -bonusKnight[1][sq]} - Score{KNIGHT_MID, KNIGHT_END};
        pst[black_bishop][s] = Score{-bonusBishop[0][sq], -bonusBishop[1][sq]} - Score{BISHOP_MID, BISHOP_END};
        pst[black_rook][s]   = Score{-bonusRook  [0][sq], -bonusRook  [1][sq]} - Score{ROOK_MID, ROOK_END};
        pst[black_queen][s]  = Score{-bonusQueen [0][sq], -bonusQueen [1][sq]} - Score{QUEEN_MID, QUEEN_END};
        pst[black_king][s]   = Score{-bonusKing  [0][sq], -bonusKing  [1][sq]} - Score{0, 0};
    }
}
