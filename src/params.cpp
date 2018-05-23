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
    PAWN_MID = 100, PAWN_END = 123,
    KNIGHT_MID = 425, KNIGHT_END = 426,
    BISHOP_MID = 430, BISHOP_END = 466,
    ROOK_MID = 703, ROOK_END = 729,
    QUEEN_MID = 1450, QUEEN_END = 1369;

Score
    protected_piece_bonus = {9, 0},
    rook_pawn_bonus = {0, 2},
    minor_piece_behind_pawn = {10, 0},
    strong_pawn_threat = {78, 27},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {6, 0},
    hanging_threat_bonus = {3, 15},
    pawn_push_threat_bonus = {29, 11};

// Penalties
Score
    double_pawn_penalty = {6, 14},
    blocked_rook_penalty = {68, 0},
    bishop_pawn_penalty = {6, 6},
    hindered_passer_penalty = {2, 0};

int
    king_only_protected_penalty = 10,
    queen_check_penalty = 36,
    knight_check_penalty = 47,
    rook_check_penalty = 46,
    bishop_check_penalty = 14,
    pawn_distance_penalty = 9,
    king_zone_attack_penalty = 4;

int pawn_shelter_penalty[8] = {0, 0, 19, 33, 35, 50, 50, 50};
int tempo = 12;
int ATTACK_VALUES[12] = {0, 0, 59, 37, 47, 9};

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {0, 11}, {0, 14}, {18, 18}, {27, 39}, {77, 113}, {100, 229} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{21, 0}, {38, 8}};

Score isolated_pawn_penalty[2] = {{16, 14}, { 3, 12}},
      backward_pawn_penalty[2] = {{25, 23}, { 6,  0}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 3, 16}, // Pawn
    {16, 42}, // Knight
    {14, 44}, // Bishop
    {50, 15}, // Rook
    {41,  3}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 3, 20}, // Pawn
    {23, 27}, // Knight
    {20, 51}, // Bishop
    { 0,  0}, // Rook
    {75, 34}  // Queen
    // { 0, 0}  // King should never be called
};
