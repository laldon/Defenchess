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

#ifndef TEXEL_H
#define TEXEL_H

#include "data.h"

// Bonuses
extern Score
    protected_piece_bonus,
    rook_pawn_bonus,
    minor_piece_behind_pawn,
    strong_pawn_threat,
    weak_pawn_threat,
    rank_threat_bonus,
    hanging_threat_bonus,
    pawn_push_threat_bonus;

// Penalties
extern Score
    double_pawn_penalty,
    blocked_rook_penalty,
    bishop_pawn_penalty,
    hindered_passer_penalty;

extern int
    king_only_protected_penalty,
    queen_check_penalty,
    knight_check_penalty,
    rook_check_penalty,
    bishop_check_penalty,
    pawn_distance_penalty,
    king_zone_attack_penalty;

const int pawn_shelter_penalty[8] = {0, 0, 10, 30, 40, 50, 50, 50};
const int tempo = 12;
const int ATTACK_VALUES[12] = {0,0,0,0,80,80,55,55,45,45,10,10};

extern Score mobility_bonus[4][32];

extern Score passed_pawn_bonus[7];

extern Score passed_file_bonus[8];

extern Score rook_file_bonus[2];

extern Score isolated_pawn_penalty[2],
             backward_pawn_penalty[2];

extern Score minor_threat_bonus[6];

extern Score rook_threat_bonus[6];

void tune();

#endif