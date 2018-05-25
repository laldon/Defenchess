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
    PAWN_MID = 100, PAWN_END = 134,
    KNIGHT_MID = 441, KNIGHT_END = 448,
    BISHOP_MID = 484, BISHOP_END = 480,
    ROOK_MID = 729, ROOK_END = 746,
    QUEEN_MID = 1533, QUEEN_END = 1400;

int piece_values[14] = {0, 0, PAWN_MID, PAWN_MID, KNIGHT_MID, KNIGHT_MID, BISHOP_MID, BISHOP_MID,
                              ROOK_MID, ROOK_MID, QUEEN_MID, QUEEN_MID, 100 * QUEEN_MID, 100 * QUEEN_MID};

Score
    protected_piece_bonus = {4, 0},
    rook_pawn_bonus = {1, 17},
    minor_piece_behind_pawn = {8, 0},
    strong_pawn_threat = {75, 32},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {3, 0},
    hanging_threat_bonus = {0, 21},
    pawn_push_threat_bonus = {28, 13};

// Penalties
Score
    double_pawn_penalty = {11, 26},
    blocked_rook_penalty = {65, 0},
    bishop_pawn_penalty = {6, 11},
    hindered_passer_penalty = {1, 0};

int
    king_only_protected_penalty = 7,
    queen_check_penalty = 32,
    knight_check_penalty = 35,
    rook_check_penalty = 38,
    bishop_check_penalty = 8,
    pawn_distance_penalty = 10,
    king_zone_attack_penalty = 5;

int pawn_shelter_penalty[8] = {0, 0, 18, 31, 28, 22, 68, 52};
int tempo = 12;
int ATTACK_VALUES[12] = {0, 0, 69, 43, 47, 9};

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {0, 18}, {0, 18}, {10, 24}, {29, 38}, {91, 91}, {202, 116} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{15, 0}, {41, 14}};

Score isolated_pawn_penalty[2] = {{11, 25}, {12, 5}},
      backward_pawn_penalty[2] = {{34, 32}, {13, 0}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 3, 24}, // Pawn
    {20, 39}, // Knight
    {39, 19}, // Bishop
    {61,  8}, // Rook
    {36, 15}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    {10, 26}, // Pawn
    {28, 33}, // Knight
    {31, 36}, // Bishop
    { 0,  0}, // Rook
    {65, 11}  // Queen
    // { 0, 0}  // King should never be called
};
