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

#include "tt.h"
#include "data.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "move_utils.h"

Table table;
PawnTTEntry *pawntt;

const uint64_t one_mb = 1024ULL * 1024ULL;

const uint64_t pawntt_size = one_mb * 1ULL; // 1 MB
const uint64_t pawntt_mod = (uint64_t)(pawntt_size / sizeof(PawnTTEntry));

void init_tt() {
    table.tt_size = one_mb * 8ULL; // 8 MB
    table.tt = (Bucket*) malloc(table.tt_size);
    table.bucket_mask = (uint64_t)(table.tt_size / sizeof(Bucket) - 1);
    table.generation = 0;
    std::memset(table.tt, 0, table.tt_size);

    pawntt = (PawnTTEntry*) malloc(pawntt_size);
    std::memset(pawntt, 0, pawntt_size);
    // std::cout << sizeof(Bucket) << std::endl;
}

void reset_tt(int megabytes) {
    table.tt_size = one_mb * (uint64_t) (megabytes);
    table.tt = (Bucket*) realloc(table.tt, table.tt_size);
    table.bucket_mask = (uint64_t)(table.tt_size / sizeof(Bucket) - 1);
}

void clear_tt() {
    std::memset(table.tt, 0, table.tt_size);
    std::memset(pawntt, 0, pawntt_size);
    table.generation = 0;
}

void start_search() {
    table.generation = (table.generation + 1) % 64;
}

int score_to_tt(int score, uint16_t ply) {
    if (score >= MATE_IN_MAX_PLY) {
        return score + ply;
    } else if (score <= MATED_IN_MAX_PLY) {
        return score - ply;
    } else {
        return score;
    }
}

int tt_to_score(int score, uint16_t ply) {
    if (score >= MATE_IN_MAX_PLY) {
        return score - ply;
    } else if (score <= MATED_IN_MAX_PLY) {
        return score + ply;
    } else {
        return score;
    }
}

int hashfull() {
    int count = 0;
    for (uint64_t i = 0; i < table.bucket_mask; i += uint64_t(table.bucket_mask / 1000)) {
        Bucket *bucket = &table.tt[i];
        for (int j = 0; j < bucket_size; ++j) {
            if (bucket->ttes[j].hash) {
                ++count;
            }
        }
    }
    return count / bucket_size;
}

int age_diff(TTEntry *tte) {
    return (table.generation - tte_age(tte)) & 0x3F;
}

void set_tte(uint64_t hash, TTEntry *tte, Move move, int depth, int score, int static_eval, uint8_t flag) {
#ifndef __TUNE__
    uint16_t h = (uint16_t)(hash >> 48);

    if (move || h != tte->hash) {
        tte->move = move;
    }

    if (h != tte->hash || depth > tte->depth - 4) {
        assert(depth < 256 && depth > -256);
        tte->hash = h;
        tte->depth = (int8_t)depth;
        tte->score = (int16_t)score;
        tte->static_eval = (int16_t)static_eval;
        tte->ageflag = (table.generation << 2) | flag;
    }
#endif
}

TTEntry *get_tte(uint64_t hash, bool &tt_hit) {
#ifndef __TUNE__
    uint64_t index = hash & table.bucket_mask;
    Bucket *bucket = &table.tt[index];

    uint16_t h = (uint16_t)(hash >> 48);
    for (int i = 0; i < bucket_size; ++i) {
        if (!bucket->ttes[i].hash) {
            tt_hit = false;
            return &bucket->ttes[i];
        }
        if (bucket->ttes[i].hash == h) {
            tt_hit = true;
            return &bucket->ttes[i];
        }
    }

    TTEntry *replacement = &bucket->ttes[0];
    for (int i = 1; i < bucket_size; ++i) {
        if (bucket->ttes[i].depth - age_diff(&bucket->ttes[i]) * 16 < replacement->depth - age_diff(replacement) * 16) {
            replacement = &bucket->ttes[i];
        }
    }

    tt_hit = false;
    return replacement;
#else
    Bucket *bucket = &table.tt[0];
    tt_hit = false;
    return &bucket->ttes[0];
#endif
}

void set_pawntte(uint64_t pawn_hash, Evaluation* eval) {
#ifndef __TUNE__
    uint64_t index = pawn_hash % pawntt_mod;
    PawnTTEntry *pawntte = &pawntt[index];
    pawntte->pawn_hash = (uint32_t)(pawn_hash >> 32);
    pawntte->score = eval->score_pawn;
    pawntte->pawn_passers[white] = eval->pawn_passers[white];
    pawntte->pawn_passers[black] = eval->pawn_passers[black];
    pawntte->semi_open_files[white] = eval->semi_open_files[white];
    pawntte->semi_open_files[black] = eval->semi_open_files[black];
#endif
}

PawnTTEntry *get_pawntte(uint64_t pawn_hash) {
#ifndef __TUNE__
    uint64_t index = pawn_hash % pawntt_mod;
    PawnTTEntry *pawntte = &pawntt[index];
    if (pawntte->pawn_hash == (uint32_t)(pawn_hash >> 32)) {
        return pawntte;
    }
#endif
    return 0;
}
