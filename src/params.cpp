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

int
    king_only_protected_penalty = 6,
    queen_check_penalty = 36,
    knight_check_penalty = 35,
    rook_check_penalty = 38,
    bishop_check_penalty = 20,
    pawn_distance_penalty = 10,
    king_zone_attack_penalty = 5;

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
