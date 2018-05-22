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
    PAWN_MID = 95, PAWN_END = 129,
    KNIGHT_MID = 399, KNIGHT_END = 441,
    BISHOP_MID = 441, BISHOP_END = 425,
    ROOK_MID = 683, ROOK_END = 737,
    QUEEN_MID = 1414, QUEEN_END = 1400;

Score
    protected_piece_bonus = {5, 0},
    rook_pawn_bonus = {6, 16},
    minor_piece_behind_pawn = {8, 0},
    strong_pawn_threat = {70, 29},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {3, 0},
    hanging_threat_bonus = {0, 24},
    pawn_push_threat_bonus = {28, 12};

// Penalties
Score
    double_pawn_penalty = {11, 24},
    blocked_rook_penalty = {64, 0},
    bishop_pawn_penalty = {4, 8},
    hindered_passer_penalty = {3, 0};

int
    king_only_protected_penalty = 7,
    queen_check_penalty = 30,
    knight_check_penalty = 30,
    rook_check_penalty = 43,
    bishop_check_penalty = 12,
    pawn_distance_penalty = 10,
    king_zone_attack_penalty = 5;

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {6, 18}, {2, 18}, {18, 23}, {37, 36}, {109, 81}, {196, 113} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{13, 0}, {38, 13}};

Score isolated_pawn_penalty[2] = {{12, 23}, {10, 6}},
      backward_pawn_penalty[2] = {{35, 31}, {13, 0}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 4, 20}, // Pawn
    {22, 40}, // Knight
    {40, 15}, // Bishop
    {58,  7}, // Rook
    {33, 15}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    {15, 20}, // Pawn
    {32, 30}, // Knight
    {33, 28}, // Bishop
    { 0,  0}, // Rook
    {60, 21}  // Queen
    // { 0, 0}  // King should never be called
};
