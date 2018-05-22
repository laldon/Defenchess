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
    PAWN_MID = 100, PAWN_END = 140,
    KNIGHT_MID = 400, KNIGHT_END = 440,
    BISHOP_MID = 430, BISHOP_END = 470,
    ROOK_MID = 650, ROOK_END = 710,
    QUEEN_MID = 1300, QUEEN_END = 1350;

Score
    protected_piece_bonus = {10, 0},
    rook_pawn_bonus = {5, 15},
    minor_piece_behind_pawn = {10, 0},
    strong_pawn_threat = {100, 100},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {10, 2},
    hanging_threat_bonus = {30, 15},
    pawn_push_threat_bonus = {20, 15};

// Penalties
Score
    double_pawn_penalty = {10, 20},
    blocked_rook_penalty = {70, 0},
    bishop_pawn_penalty = {5, 5},
    hindered_passer_penalty = {5, 0};

int
    king_only_protected_penalty = 11,
    queen_check_penalty = 50,
    knight_check_penalty = 50,
    rook_check_penalty = 55,
    bishop_check_penalty = 30,
    pawn_distance_penalty = 10,
    king_zone_attack_penalty = 6;

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {3, 3}, {5, 5}, {20, 20}, {45, 45}, {100, 100}, {150, 150} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{12, 4}, {26, 12}};

Score isolated_pawn_penalty[2] = {{16, 18}, { 8, 11}},
      backward_pawn_penalty[2] = {{23, 15}, {14,  7}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0, 10}, // Pawn
    {20, 20}, // Knight
    {20, 20}, // Bishop
    {30, 40}, // Rook
    {30, 40}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0, 10}, // Pawn
    {20, 35}, // Knight
    {20, 35}, // Bishop
    { 0,  0}, // Rook
    {20, 30}  // Queen
    // { 0, 0}  // King should never be called
};
