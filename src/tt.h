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

#ifndef TT_H
#define TT_H

#include "data.h"

void init_tt();
void clear_tt();
void reset_tt(int megabytes);

extern uint64_t tt_size;
extern uint64_t tt_mask;

typedef struct TTEntry {
    uint32_t hash;
    Move     move;
    int8_t  depth;
    int      score;
    uint8_t  flag;
} TTEntry;

typedef struct PawnTTEntry {
    uint32_t pawn_hash;
    Score    score;
    Bitboard pawn_passers[2];
    int      semi_open_files[2];
} PawnTTEntry;

int hashfull();
void set_tte(uint64_t hash, Move m, int depth, int score, uint8_t flag);
TTEntry *get_tte(uint64_t hash);

void set_pawntte(uint64_t pawn_hash, Evaluation* eval);
PawnTTEntry *get_pawntte(uint64_t pawn_hash);

int score_to_tt(int score, uint16_t ply);
int tt_to_score(int score, uint16_t ply);

extern TTEntry *tt;
extern PawnTTEntry *pawntt;

#endif
