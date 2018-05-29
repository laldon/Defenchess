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

#include "params.h"

int
    PAWN_MID = 100, PAWN_END = 136,
    KNIGHT_MID = 442, KNIGHT_END = 446,
    BISHOP_MID = 487, BISHOP_END = 478,
    ROOK_MID = 731, ROOK_END = 742,
    QUEEN_MID = 1416, QUEEN_END = 1424;

int piece_values[14] = {0, 0, PAWN_MID, PAWN_MID, KNIGHT_MID, KNIGHT_MID, BISHOP_MID, BISHOP_MID,
                              ROOK_MID, ROOK_MID, QUEEN_MID, QUEEN_MID, 100 * QUEEN_MID, 100 * QUEEN_MID};

Score
    protected_piece_bonus = {12, 0},
    rook_pawn_bonus = {3, 18},
    minor_piece_behind_pawn = {9, 0},
    strong_pawn_threat = {76, 32},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {2, 0},
    hanging_threat_bonus = {2, 21},
    pawn_push_threat_bonus = {25, 16};

// Penalties
Score
    double_pawn_penalty = {4, 30},
    blocked_rook_penalty = {65, 0},
    bishop_pawn_penalty = {6, 11},
    hindered_passer_penalty = {1, 0};

// King safety
int
    king_only_protected_penalty = 7,
    queen_check_penalty = 32,
    knight_check_penalty = 33,
    rook_check_penalty = 38,
    bishop_check_penalty = 23,
    pawn_distance_penalty = 6,
    king_zone_attack_penalty = 5,
    pawn_shelter_divisor = 10,
    queen_number_mult = 54,
    king_danger_mult = 10,
    king_zone_divisor = 14,
    pinned_bonus = 6,
    king_danger_divisor_mid = 29,
    king_danger_divisor_end = 2;

int pawn_shelter_penalty[8] = {0, 0, 20, 31, 32, 50, 50, 50};
int tempo = 12;
int ATTACK_VALUES[12] = {0, 0, 74, 42, 35, 10};

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {0, 10}, {0, 12}, {12, 18}, {35, 30}, {107, 79}, {205, 107} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{15, 0}, {39, 14}};

Score isolated_pawn_penalty[2] = {{11, 24}, {12, 3}},
      backward_pawn_penalty[2] = {{35, 30}, {11, 0}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 4, 22}, // Pawn
    {29, 37}, // Knight
    {44, 20}, // Bishop
    {67,  4}, // Rook
    {37, 18}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    {14, 24}, // Pawn
    {33, 33}, // Knight
    {37, 34}, // Bishop
    { 0,  0}, // Rook
    {63,  4}  // Queen
    // { 0, 0}  // King should never be called
};

Score pst[14][64];

int bonusPawn[2][32] = {
    {
        0,    0,    0,    0,
        2,  -44,   -6,   18,
      -19,   -2,   12,   18,
        1,    1,    6,   24,
       -7,    0,    7,   20,
       -1,   -3,   -2,    5,
      -13,    0,   -3,   -5,
        0,    0,    0,    0,
    }, {
        0,    0,    0,    0,
      -39,   21,   28,   20,
      -12,    2,   -3,    6,
        2,   -2,   -4,   -6,
        1,   -3,   -5,   -9,
        2,   -2,    1,    5,
       -1,   -1,   10,    4,
        0,    0,    0,    0,
    }
};

int bonusKnight[2][32] = {
    {
     -270, -146,  -90,   11,
      -62,  -26,    4,   29,
       -7,   24,   34,   34,
       11,   15,   29,   30,
       -5,    7,   18,   22,
      -26,   -7,   -8,    5,
      -32,  -23,  -11,   -9,
      -57,  -39,  -24,   -9,
    }, {
      -62,  -20,  -12,   -7,
       -9,    0,    0,   16,
      -14,    9,   20,   12,
        4,    1,   20,   26,
       -9,   11,   14,   22,
      -33,  -14,  -13,   12,
      -50,  -10,  -21,  -10,
      -34,  -36,  -28,  -22,
    }
};

int bonusBishop[2][32] = {
    {
        3,  -31, -109,  -12,
      -47,  -65,  -23,  -40,
      -18,   40,   26,    5,
       -5,   -1,   18,   13,
        2,    8,    5,   18,
       -5,    7,    6,    1,
        3,    9,   17,   -4,
       -1,   15,  -13,  -11,
    }, {
      -19,    5,  -12,   -1,
      -13,   -1,    6,   10,
        0,    0,    3,   11,
      -11,    4,   -4,    5,
      -16,  -11,    4,   -5,
      -25,  -12,  -10,    3,
      -33,  -26,  -27,   -8,
      -41,  -40,  -20,  -17,
    }
};

int bonusRook[2][32] = {
    {
       32,   12,   -9,   23,
        6,  -16,    7,   25,
        7,   14,    6,   27,
        2,    7,   17,    6,
      -22,    0,  -14,    2,
      -31,  -11,  -14,  -15,
      -46,  -15,   -1,   -6,
      -13,  -12,   -3,    1,
    }, {
       15,   22,   12,   10,
       13,   20,   16,   11,
       11,   15,   16,    8,
       12,   17,   17,   15,
        9,    9,   13,    7,
       -4,   -3,   -6,   -8,
      -16,  -14,  -14,  -12,
       -1,   -9,   -9,  -14,
    }
};

int bonusQueen[2][32] = {
    {
      -13,   12,   11,   11,
      -16,  -45,  -24,  -28,
       13,   -7,  -22,  -24,
       -1,  -19,    0,  -33,
       -8,   12,    0,   -4,
       -1,    7,   10,   -1,
       10,    2,   20,    5,
       18,   -7,   -7,   -5,
    }, {
       16,  -10,   11,   19,
       16,   37,   28,   38,
       -8,   13,   31,   32,
       22,   46,   22,   46,
        7,    9,    9,   11,
      -19,   -5,   -8,   -7,
      -58,  -38,  -47,  -17,
      -67,  -44,  -42,  -27,
    }
};

int bonusKing[2][32] = {
    {
       60,  266,  227,  326,
      231,  348,  202,  220,
      182,  310,  266,  268,
      170,  423,  331,  267,
      108,  243,  229,  233,
      100,  140,  163,  175,
      145,  152,  125,  119,
      137,  179,  155,  113,
    }, {
      -53,  154,  117,   83,
       79,  113,  115,  101,
      119,  111,  122,  102,
       94,   93,  110,  110,
       71,   90,   97,  107,
       60,   76,   83,   91,
       52,   63,   73,   75,
       24,   43,   52,   57,
    }
};

