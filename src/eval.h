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

#ifndef EVAL_H
#define EVAL_H

#include "data.h"
#include <sstream>
#include <iomanip>

void print_eval(Position *p);

// Bonuses
const Score
    protected_piece_bonus = {10, 0},
    rook_pawn_bonus = {5, 15},
    minor_piece_behind_pawn = {10, 0},
    strong_pawn_threat = {100, 100},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {10, 2},
    hanging_threat_bonus = {30, 15},
    pawn_push_threat_bonus = {20, 15};

// Penalties
const Score
    double_pawn_penalty = {10, 20},
    blocked_rook_penalty = {70, 0},
    bishop_pawn_penalty = {5, 5},
    hindered_passer_penalty = {5, 0};

const int
    king_only_protected_penalty = 11,
    queen_check_penalty = 50,
    knight_check_penalty = 50,
    rook_check_penalty = 55,
    bishop_check_penalty = 30,
    pawn_distance_penalty = 10,
    king_zone_attack_penalty = 6;

const int pawn_shelter_penalty[8] = {0, 0, 10, 30, 40, 50, 50, 50};
const int tempo = 12;
const int ATTACK_VALUES[12] = {0,0,0,0,80,80,55,55,45,45,10,10};

int evaluate(Position *p, int *opponent_king_eval);
void init_eval();

inline Color winning_side(Position *p) {
    return p->score.endgame > 0 ? white : black;
}

inline std::string score_str(Score sc){
    std::stringstream stream1, stream2;
    stream1 << std::fixed << std::setprecision(2) << (double) sc.midgame / (double) PAWN_END ;
    stream2 << std::fixed << std::setprecision(2) << (double) sc.endgame / (double) PAWN_END ;
    if (sc.midgame < 0 && sc.endgame < 0)
        return "" + stream1.str() + " :" + stream2.str() + " ";
    if (sc.midgame < 0 && sc.endgame >= 0)
        return "" + stream1.str() + " : " + stream2.str() + " ";
    if (sc.midgame >= 0 && sc.endgame < 0)
        return " " + stream1.str() + " :" + stream2.str() + " ";
    return " " + stream1.str() + " : " + stream2.str() + " ";
}

#endif
