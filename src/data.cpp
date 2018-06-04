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

#include "data.h"
#include "bitboard.h"
#include "test.h"
#include "tt.h"
#include "pst.h"
#include "magic.h"
#include "eval.h"
#include "endgame.h"
#include <cmath>
#include "bitbase.h"

Bitboard ROOK_MASKS[64];
Bitboard BISHOP_MASKS_1[64];
Bitboard BISHOP_MASKS_2[64];
Bitboard BISHOP_MASKS_COMBINED[64];
Bitboard ROOK_MASKS_HORIZONTAL[64];
Bitboard ROOK_MASKS_VERTICAL[64];
Bitboard ROOK_MASKS_COMBINED[64];
Bitboard KNIGHT_MASKS[64];
Bitboard KING_MASKS[64];
Bitboard KING_EXTENDED_MASKS[2][64];
Bitboard bfi[64];
Bitboard bfi_enpassant[64];
Bitboard bfi_queen_castle[2];
Bitboard bfi_king_castle[2];
Bitboard KING_CASTLE_MASK[2];
Bitboard QUEEN_CASTLE_MASK[2];
Bitboard PAWN_ADVANCE_MASK_1[64][2];
Bitboard PAWN_ADVANCE_MASK_2[64][2];
Bitboard PAWN_CAPTURE_MASK[64][2];
Bitboard BETWEEN_MASK[64][64];
Bitboard FROMTO_MASK[64][64];
Bitboard PASSED_PAWN_MASK[64][2];
Bitboard FRONT_MASK[64][2];
Bitboard ADJACENT_MASK[64];
Bitboard CENTER_MASK;
Bitboard FRONT_KING_RANK1[2][64];
Bitboard FRONT_KING_RANK2[2][64];
Bitboard DISTANCE_RING[64][8];

Material material_base[9*3*3*3*2*9*3*3*3*2];

uint8_t timer_count = 0;

uint8_t CASTLING_RIGHTS[64];

uint8_t ROOK_MOVES_CASTLE_FROM[64];
uint8_t ROOK_MOVES_CASTLE_TO[64];
uint8_t ROOK_MOVES_CASTLE_PIECE[64];

uint8_t ENPASSANT_INDEX[64];
uint64_t castlingHash[16];
uint64_t polyglotCombined[14][64];

int num_threads = 1;
int move_overhead = 100;

SearchThread search_threads[MAX_THREADS];

int mvvlva_values[12][14];

int reductions[2][64][64];

PV pv[MAX_PLY + 1];
PV debug_pv;

void get_ready() {
    // Clear pv
    for (int i = 0; i < MAX_PLY; i++) {
        pv[i].size = 0;
    }

    SearchThread *main_thread = &search_threads[0];
    main_thread->root_ply = main_thread->search_ply;

    // Copy over the root position
    for (int i = 1; i < MAX_THREADS; ++i) {
        SearchThread *t = &(search_threads[i]);
        t->root_ply = t->search_ply = main_thread->root_ply;

        // Need to fully copy the position
        std::memcpy(t->positions + main_thread->search_ply, main_thread->positions + main_thread->search_ply, sizeof(Position));
        Position *p = &(t->positions[t->search_ply]);
        p->my_thread = t;
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        SearchThread *t = &(search_threads[i]);
        t->depth = 1;

        // Clear the metadata
        for (int j = 0; j < MAX_PLY + 1; ++j) {
            Metadata *md = &t->metadatas[j];
            md->current_move = no_move;
            md->static_eval = UNDEFINED;
            md->ply = 0;
            md->killers[0] = no_move;
            md->killers[1] = no_move;
        }
        // Clear counter moves
        for (int j = 0; j < 14; ++j) {
            for (int k = 0; k < 64; ++k) {
                t->counter_moves[j][k] = 0;
            }
        }
        // Clear history
        for (int j = 0; j < 14; ++j) {
            for (int k = 0; k < 64; ++k) {
                t->history[j][k] = 0;
            }
        }
    }
}

Bitboard knight_possibles(uint64_t index) {
    switch(index & 7){
        case 0:
            return (_rook_vertical) | (_rook_vertical << 1) | (_rook_vertical << 2);
        case 1:
            return (_rook_vertical) | (_rook_vertical << 1) | (_rook_vertical << 2) | (_rook_vertical << 3);
        case 6:
            return (_rook_vertical << 4) | (_rook_vertical << 5) | (_rook_vertical << 6) | (_rook_vertical << 7);
        case 7:
            return (_rook_vertical << 5) | (_rook_vertical << 6) | (_rook_vertical << 7);
        default:
            return 0xFFFFFFFFFFFFFFFF;
    }
}

void init_fromto() {
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            if (i == j) {
                FROMTO_MASK[i][j] = 0;
                continue;
            }
            if (i % 8 == j % 8) {
                FROMTO_MASK[i][j] = (line_vertical << i%8);
            } else if (i / 8 == j / 8) {
                FROMTO_MASK[i][j] = (line_horizontal << 8*(i/8));
            } else if (i / 8 - j / 8 == i % 8 - j % 8 || i / 8 - j / 8 == j % 8 - i % 8) {
                if (i > j)
                    FROMTO_MASK[i][j] = (i % 8 - j % 8 < 0 ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i]);
                else
                    FROMTO_MASK[i][j] = (i % 8 - j % 8 > 0 ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i]);
            } else {    
                FROMTO_MASK[i][j] = 0;
            }
        }
    }
}

void init_between() {
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            if (i == j) {
                BETWEEN_MASK[i][j] = 0;
                continue;
            }
            if (i % 8 == j % 8) {
                if (i > j)
                    BETWEEN_MASK[i][j] = (bfi[i] - 2*bfi[j]) & (line_vertical << i%8);
                else
                    BETWEEN_MASK[i][j] = (bfi[j] - 2*bfi[i]) & (line_vertical << i%8);
            } else if (i / 8 == j / 8) {
                if (i > j)
                    BETWEEN_MASK[i][j] = (bfi[i] - 2*bfi[j]);
                else
                    BETWEEN_MASK[i][j] = (bfi[j] - 2*bfi[i]);
            } else if (i / 8 - j / 8 == i % 8 - j % 8 || i / 8 - j / 8 == j % 8 - i % 8) {
                if (i > j)
                    BETWEEN_MASK[i][j] = (bfi[i] - 2 * bfi[j]) & (i % 8 - j % 8 < 0 ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i]);
                else
                    BETWEEN_MASK[i][j] = (bfi[j] - 2* bfi[i]) & (i % 8 - j % 8 > 0 ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i]);
            } else {    
                BETWEEN_MASK[i][j] = 0;
            }
        }
    }
}

void init_pawns(){
    for (int i = 0; i < 64; i++) {
        PAWN_ADVANCE_MASK_1[i][white] = 0;
        PAWN_ADVANCE_MASK_2[i][white] = 0;
        PAWN_CAPTURE_MASK[i][white] = 0;
        PAWN_ADVANCE_MASK_1[i][black] = 0;
        PAWN_ADVANCE_MASK_2[i][black] = 0;
        PAWN_CAPTURE_MASK[i][black] = 0;
    }
    for (int i = 8; i < 56; i++) {
        PAWN_ADVANCE_MASK_1[i][white] = bfi[i + 8];
        PAWN_ADVANCE_MASK_1[i][black] = bfi[i - 8];
        if (row((Square)i) == 1) {
            PAWN_ADVANCE_MASK_2[i][white] |= bfi[i + 16];
        }
        if (row((Square)i) == 6) {
            PAWN_ADVANCE_MASK_2[i][black] |= bfi[i - 16];
        }
    }
    for (int i = 0; i < 64; i++) {
        PAWN_CAPTURE_MASK[i][white] = 0;
        PAWN_CAPTURE_MASK[i][black] = 0;
        int column = col(i);
        if (column != 0) {
            if (i + 7 <= 63)
                PAWN_CAPTURE_MASK[i][white] |= bfi[i + 7];
            if (i - 9 >= 0)
                PAWN_CAPTURE_MASK[i][black] |= bfi[i - 9];
        }
        if (column != 7) {
            if (i + 9 <= 63)
                PAWN_CAPTURE_MASK[i][white] |= bfi[i + 9];
            if (i - 7 >= 0)
                PAWN_CAPTURE_MASK[i][black] |= bfi[i - 7];
        }
    }
}

void init_enpassants(){
    for (int i = 0; i < 64; i++) {
        if (i >= 8 && i <= 23) {
            ENPASSANT_INDEX[i] = i + 8;
        }
        if (i >= 40 && i <= 55) {
            ENPASSANT_INDEX[i] = i - 8;
        }
    }
}

void init_knight_masks() {
    for (int i = 0; i < 64; i++) {
        KNIGHT_MASKS[i] = shift(_knight_targets, i - 21) & knight_possibles(i);
    }
}

void init_king_masks(){
    for (int i = 0; i < 64; i++) {
        KING_MASKS[i] = shift(_king_targets, i - 21) & knight_possibles(i);
    }
}

void init_bishop_masks() {
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 15; j++) {
            if (bfi[i] & cross_lt[j])
                BISHOP_MASKS_1[i] = cross_lt[j];
            if (bfi[i] & cross_rt[j])
                BISHOP_MASKS_2[i] = cross_rt[j];
            
        }
    }
    for (int i = 0; i < 64; i++){
        BISHOP_MASKS_COMBINED[i] = BISHOP_MASKS_1[i] | BISHOP_MASKS_2[i];
    }
}

void init_rook_masks() {
    for (int i = 0; i < 64; i++) {
       ROOK_MASKS_HORIZONTAL[i] = line_horizontal << (i & 0xFFFFFFFFFFFFFFF8);
       ROOK_MASKS_VERTICAL[i] = line_vertical << (i & 7);
       ROOK_MASKS_COMBINED[i] = ROOK_MASKS_VERTICAL[i] | ROOK_MASKS_HORIZONTAL[i];
    }
}

void init_bfi() {
    for (int i = 0; i < 64; i++) {
        bfi[i] = 1ULL << i;
        if (i != 0) {
            bfi_enpassant[i] = 1ULL << i;
        }
    }
    
    CENTER_MASK = bfi[D4] | bfi[E4] | bfi[D5] | bfi[E5];    
}

void init_castles() {
    KING_CASTLE_MASK[white] = bfi[F1] | bfi[G1];
    KING_CASTLE_MASK[black] = bfi[F8] | bfi[G8];

    QUEEN_CASTLE_MASK[white] = bfi[B1] | bfi[C1] | bfi[D1];
    QUEEN_CASTLE_MASK[black] = bfi[B8] | bfi[C8] | bfi[D8];

    bfi_queen_castle[white] = bfi[C1];
    bfi_queen_castle[black] = bfi[C8];
    bfi_king_castle[white] = bfi[G1];
    bfi_king_castle[black] = bfi[G8];

    // black_queenside | black_kingside | white_queenside | white_kingside
    for (int i = 0; i < 64; i++) {
        if (i == A8) {
            // Black queenside
            CASTLING_RIGHTS[i] = 7;
        } else if (i == H8) {
            // Black kingside
            CASTLING_RIGHTS[i] = 11;
        } else if (i == E8) {
            // Black both
            CASTLING_RIGHTS[i] = 3;
        } else if (i == A1) {
            // White queenside
            CASTLING_RIGHTS[i] = 13;
        } else if (i == H1) {
            // White kingside
            CASTLING_RIGHTS[i] = 14;
        } else if (i == E1) {
            // White both
            CASTLING_RIGHTS[i] = 12;
        } else {
            CASTLING_RIGHTS[i] = 15;
        }
        ROOK_MOVES_CASTLE_FROM[i] = 0;
        ROOK_MOVES_CASTLE_TO[i] = 0;
        ROOK_MOVES_CASTLE_PIECE[i] = 0;
    }

    ROOK_MOVES_CASTLE_FROM[G1] = H1;
    ROOK_MOVES_CASTLE_TO[G1] = F1;
    ROOK_MOVES_CASTLE_PIECE[G1] = white_rook;
    ROOK_MOVES_CASTLE_FROM[C1] = A1;
    ROOK_MOVES_CASTLE_TO[C1] = D1;
    ROOK_MOVES_CASTLE_PIECE[C1] = white_rook;
    ROOK_MOVES_CASTLE_FROM[G8] = H8;
    ROOK_MOVES_CASTLE_TO[G8] = F8;
    ROOK_MOVES_CASTLE_PIECE[G8] = black_rook;
    ROOK_MOVES_CASTLE_FROM[C8] = A8;
    ROOK_MOVES_CASTLE_TO[C8] = D8;
    ROOK_MOVES_CASTLE_PIECE[C8] = black_rook;
}

void init_polyglot() {
    for (int castle_mask = 0; castle_mask < 16; castle_mask++) {
        if (castle_mask & 1) { // White king-side
            castlingHash[castle_mask] ^= polyglotCastle[0];
        }
        if (castle_mask & 2) { // White queen-side
            castlingHash[castle_mask] ^= polyglotCastle[1];
        }
        if (castle_mask & 4) { // Black king-side
            castlingHash[castle_mask] ^= polyglotCastle[2];
        }
        if (castle_mask & 8) { // Black queen-side
            castlingHash[castle_mask] ^= polyglotCastle[3];
        }
    }

    for (int i = 0; i < 14; i++) {
        for (int j = 0; j < 64; j++) {
            polyglotCombined[i][j] = polyglotArray[polyglotPieces[i] + j];
        }
    }
}

void init_passed_pawns() {
    Bitboard PASSED_PAWN_HORIZONTAL[64][2];
    for (int i = 0; i < 64; i++) {
        PASSED_PAWN_HORIZONTAL[i][white] = 0;
        PASSED_PAWN_HORIZONTAL[i][black] = 0;
        for (int j = i + 8; j < 64; j += 8) {
            PASSED_PAWN_HORIZONTAL[i][white] |= ROOK_MASKS_HORIZONTAL[j];
        }
        for (int j = i - 8; j >= 0; j -= 8) {
            PASSED_PAWN_HORIZONTAL[i][black] |= ROOK_MASKS_HORIZONTAL[j];
        }
    }
    for (int i = 0; i < 64; i++) {
        PASSED_PAWN_MASK[i][white] = ROOK_MASKS_VERTICAL[i];
        PASSED_PAWN_MASK[i][black] = ROOK_MASKS_VERTICAL[i];
        if (col(i) != 0) {
            PASSED_PAWN_MASK[i][white] |= ROOK_MASKS_VERTICAL[i - 1];
            PASSED_PAWN_MASK[i][black] |= ROOK_MASKS_VERTICAL[i - 1];
        }
        if (col(i) != 7) {
            PASSED_PAWN_MASK[i][white] |= ROOK_MASKS_VERTICAL[i + 1];
            PASSED_PAWN_MASK[i][black] |= ROOK_MASKS_VERTICAL[i + 1];
        }
        PASSED_PAWN_MASK[i][white] &= PASSED_PAWN_HORIZONTAL[i][white];
        PASSED_PAWN_MASK[i][black] &= PASSED_PAWN_HORIZONTAL[i][black];
    }
}

void init_pawn_masks() {
    for (int i = 0; i < 64; i++) {
        FRONT_MASK[i][white] = 0;
        FRONT_MASK[i][black] = 0;

        for (int j = i + 8; j < 64; j += 8) {
            FRONT_MASK[i][white] |= bfi[j];
        }
        for (int j = i - 8; j >= 0; j -= 8) {
            FRONT_MASK[i][black] |= bfi[j];
        }
    }
}

void init_adj(){
    for (int i = 0; i < 64 ; i++) {
        if (i % 8 == 0)
            ADJACENT_MASK[i] = bfi[i+1];
        else if (i % 8 == 7)
            ADJACENT_MASK[i] = bfi[i-1];
        else
            ADJACENT_MASK[i] = bfi[i-1] | bfi[i+1];
    }
}

void init_king_extended(){
    for (int i = 0; i < 64; i++) {
        KING_EXTENDED_MASKS[white][i] = (KING_MASKS[i] | (KING_MASKS[i] << 8)) & ~bfi[i];
        KING_EXTENDED_MASKS[black][i] = (KING_MASKS[i] | (KING_MASKS[i] >> 8)) & ~bfi[i];
    }
}

void init_king_fronts(){
    for (int i = 0 ; i < 64 ; i++) {
        FRONT_KING_RANK2[white][i] = 0;
        FRONT_KING_RANK2[black][i] = 0;
        FRONT_KING_RANK1[white][i] = 0;
        FRONT_KING_RANK1[black][i] = 0;

        if (row(i) < 7) {
            FRONT_KING_RANK1[white][i] |= bfi[i + 8];
            if (col(i) != 0)
                FRONT_KING_RANK1[white][i] |= bfi[i + 7];
            if (col(i) != 7)
                FRONT_KING_RANK1[white][i] |= bfi[i + 9];
        }

        if (row(i) < 6) {
            FRONT_KING_RANK2[white][i] |= bfi[i + 16];
            if (col(i) != 0)
                FRONT_KING_RANK2[white][i] |= bfi[i + 15];
            if (col(i) != 7)
                FRONT_KING_RANK2[white][i] |= bfi[i + 17];
        }

        if (row(i) > 0) {
            FRONT_KING_RANK1[black][i] |= bfi[i - 8];
            if (col(i) != 0)
                FRONT_KING_RANK1[black][i] |= bfi[i - 9];
            if (col(i) != 7)
                FRONT_KING_RANK1[black][i] |= bfi[i - 7];
        }

        if (row(i) > 1) {
            FRONT_KING_RANK2[black][i] |= bfi[i - 16];
            if (col(i) != 0)
                FRONT_KING_RANK2[black][i] |= bfi[i - 17];
            if (col(i) != 7)
                FRONT_KING_RANK2[black][i] |= bfi[i - 15];
        }
        // std::cout << "FRONT KING 2[black][" << i << "]:\n" << bitstring(FRONT_KING_RANK2[black][i]);
    }
}

const int ours[][6] = {
    // pair pawn knight bishop rook queen
    {   974                               }, // Bishop pair
    {    23,   0                          }, // Pawn
    {    18, 149,    -1                   }, // Knight
    {     0,  60,     2,     0            }, // Bishop
    {   -15,  -1,    27,    61, -87       }, // Rook
    {  -108,  14,    71,    80, -78,    0 }  // Queen
};


const int theirs[][6] = {
    // pair pawn knight bishop rook queen
    {     0                               }, // Bishop pair
    {    21,   0                          }, // Pawn
    {     5,  36,    0                    }, // Knight
    {    34,  38,   24,      0            }, // Bishop
    {    26,  22,   14,    -14,   0       }, // Rook
    {    59,  58,  -21,     82, 156,    0 }  // Queen
};

const int pawn_set[] = { 14, -18, 62, -29, 68, -5, -73, -12, 18 };
const int queen_minors[13] = { 18, -4, -8, -14, -2, 0, 0, 0, 0, 0, 0, 0, 0 };

int imbalance(const int piece_count[][6], Color color) {
    const Color opp_c = opponent_color(color);

    int bonus = pawn_set[piece_count[color][1]];

    // Second-degree polynomial material imbalance by Tord Romstad
    for (int pt1 = 0; pt1 <= 5; ++pt1) {
        if (!piece_count[color][pt1])
            continue;

        int v = 0;
        for (int pt2 = 0; pt2 <= pt1; ++pt2)
            v +=  ours[pt1][pt2] * piece_count[color][pt2] + theirs[pt1][pt2] * piece_count[opp_c][pt2];

        bonus += piece_count[color][pt1] * v;
    }

    // Special handling of Queen vs. Minors
    if  (piece_count[color][5] == 1 && piece_count[opp_c][5] == 0)
         bonus += queen_minors[piece_count[opp_c][2] + piece_count[opp_c][3]];

    return bonus;
}

void init_imbalance(){
    for (int wp = 0 ; wp < 9 ; wp++) {
        for (int wn = 0 ; wn < 3 ; wn++) {
            for (int wb = 0 ; wb < 3 ; wb++) {
                for (int wr = 0 ; wr < 3 ; wr++) {
                    for (int wq = 0 ; wq < 2 ; wq++){
                        for (int bp = 0 ; bp < 9 ; bp++) {
                            for (int bn = 0 ; bn < 3 ; bn++) {
                                for (int bb = 0 ; bb < 3 ; bb++) {
                                    for (int br = 0 ; br < 3 ; br++) {
                                        for (int bq = 0 ; bq < 2 ; bq++){

        int index = wq * material_balance[white_queen]  +
                    bq * material_balance[black_queen]  +
                    wr * material_balance[white_rook]   +
                    br * material_balance[black_rook]   +
                    wb * material_balance[white_bishop] +
                    bb * material_balance[black_bishop] +
                    wn * material_balance[white_knight] +
                    bn * material_balance[black_knight] +
                    wp * material_balance[white_pawn]   +
                    bp * material_balance[black_pawn];
        Material *material = &material_base[index];
        material->phase = std::max(0, (11 * (wn + wb + bn + bb) + 22 * (wr + br) + 40 * (wq + bq) - 48)) * 16 / 13;
        material->score = 0;

        const int piece_count[2][6] = {
            { wb > 1, wp, wn, wb, wr, wq },
            { bb > 1, bp, bn, bb, br, bq }
        };

        material->score = ((imbalance(piece_count, white) - imbalance(piece_count, black)) / 16);

        // Endgames
        int white_minor = wn + wb;
        int white_major = wr + wq;
        int black_minor = bn + bb;
        int black_major = br + bq;
        int all_minor = white_minor + black_minor;
        int all_major = white_major + black_major;
        bool no_pawns = wp == 0 && bp == 0;
        bool only_one_pawn = wp + bp == 1;

        // Default endgame and scaling factor types
        material->endgame_type = NORMAL_ENDGAME;
        material->scaling_factor_type = NO_SCALING;

        if (wp + bp + all_minor + all_major == 0) {
            material->endgame_type = DRAW_ENDGAME;
        }
        else if (no_pawns && all_major == 0 && white_minor < 2 && black_minor < 2) {
            material->endgame_type = DRAW_ENDGAME;
        }
        else if (no_pawns && all_major == 0 && all_minor == 2 && (wn == 2 || bn == 2)) {
            material->endgame_type = DRAW_ENDGAME;
        }
        else if (only_one_pawn && !all_minor && !all_major) {
            material->endgame_type = KNOWN_ENDGAME_KPK;
        }
        else if (no_pawns && !all_minor && all_major == 1) {
            material->endgame_type = KNOWN_ENDGAME_KXK;
        }
        else if (br == 1 && wr == 1 && only_one_pawn && !all_minor && wq == 0 && bq == 0) {
            material->scaling_factor_type = KRPKR_SCALING;
        }
                                        }
                                    }
                                }
                            }
                        }  
                    }
                }
            }
        }
    }
}

void init_distance() {
    for (Square s1 = A1; s1 <= H8; ++s1) {
        for (Square s2 = A1; s2 <= H8; ++s2) {
            if (s1 != s2)
            {
                int col_distance = std::abs(col(s1) - col(s2));
                int row_distance = std::abs(row(s1) - row(s2));
                int distance = std::max(col_distance, row_distance);
                DISTANCE_RING[s1][distance - 1] |= bfi[s2];
            }
        }
    }
}

void init_lmr() {
    for (int depth = 1; depth < 64; ++depth) {
        for (int num_moves = 1; num_moves < 64; ++num_moves) {
            reductions[0][depth][num_moves] = int(log(depth) * log(num_moves) / 1.95);
            reductions[1][depth][num_moves] = std::max(reductions[0][depth][num_moves] - 1, 0);
        }
    }
}

void init_threads() {
    for (int i = 0; i < MAX_THREADS; ++i) {
        SearchThread *search_thread = &search_threads[i];
        std::memset(search_thread->positions, 0, sizeof(search_thread->positions));
        search_thread->thread_id = i;
        search_thread->search_ply = 0;
        search_thread->root_ply = 0;
        search_thread->depth = 1;
    }
}

void init_masks() {
    init_bfi();
    init_rook_masks();
    init_king_masks();
    init_knight_masks();
    init_bishop_masks();
    init_rook_masks();
    init_castles();
    init_enpassants();
    init_between();
    init_fromto();
    init_pawns();
    init_values();
    init_polyglot();
    init_passed_pawns();
    init_pawn_masks();
    init_adj();
    init_king_extended();
    init_king_fronts();
    init_distance();
    init_lmr();
}

void init() {
    init_masks();
    init_tt();
    init_magic();
    init_eval();
    init_imbalance();
    generate_bitbase();
    init_threads();
}
