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
    PAWN_MID = 100, PAWN_END = 124,
    KNIGHT_MID = 430, KNIGHT_END = 426,
    BISHOP_MID = 430, BISHOP_END = 470,
    ROOK_MID = 707, ROOK_END = 734,
    QUEEN_MID = 1300, QUEEN_END = 1399;

Score
    protected_piece_bonus = {10, 0},
    rook_pawn_bonus = {0, 0},  // BUG?
    minor_piece_behind_pawn = {10, 0},
    strong_pawn_threat = {81, 22},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {6, 0},
    hanging_threat_bonus = {3, 17},
    pawn_push_threat_bonus = {30, 12};

// Penalties
Score
    double_pawn_penalty = {4, 15},
    blocked_rook_penalty = {72, 0},
    bishop_pawn_penalty = {0, 9},
    hindered_passer_penalty = {2, 0};

int
    king_only_protected_penalty = 11,
    queen_check_penalty = 38,
    knight_check_penalty = 46,
    rook_check_penalty = 47,
    bishop_check_penalty = 13,
    pawn_distance_penalty = 9,
    king_zone_attack_penalty = 3;

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {0, 11}, {0, 12}, {18, 19}, {31, 38}, {94, 115}, {110, 231} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{23, 0}, {40, 8}};

Score isolated_pawn_penalty[2] = {{16, 13}, { 3, 12}},
      backward_pawn_penalty[2] = {{26, 23}, { 7,  0}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 3, 16}, // Pawn
    {17, 44}, // Knight
    {14, 47}, // Bishop
    {54, 16}, // Rook
    {43,  0}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 3, 20}, // Pawn
    {25, 26}, // Knight
    {21, 51}, // Bishop
    { 0,  0}, // Rook
    {80, 29}  // Queen
    // { 0, 0}  // King should never be called
};
