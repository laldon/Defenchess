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

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <ctime>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <thread>
#include "params.h"

typedef uint8_t Square;
typedef uint64_t Bitboard;
typedef uint8_t Piece;
typedef uint16_t Move;
typedef uint8_t Color;

const Color white = 0;
const Color black = 1;

typedef struct Position Position;
typedef struct MoveGen MoveGen;
typedef struct SearchThread SearchThread;

extern Bitboard ROOK_MASKS[64];
extern Bitboard BISHOP_MASKS_1[64];
extern Bitboard BISHOP_MASKS_2[64];
extern Bitboard BISHOP_MASKS_COMBINED[64];
extern Bitboard ROOK_MASKS_HORIZONTAL[64];
extern Bitboard ROOK_MASKS_VERTICAL[64];
extern Bitboard ROOK_MASKS_COMBINED[64];
extern Bitboard KNIGHT_MASKS[64];
extern Bitboard KING_MASKS[64];
extern Bitboard KING_EXTENDED_MASKS[2][64];
extern Bitboard bfi[64];
extern Bitboard bfi_enpassant[64];
extern Bitboard bfi_queen_castle[2];
extern Bitboard bfi_king_castle[2];
extern Bitboard KING_CASTLE_MASK[2];
extern Bitboard QUEEN_CASTLE_MASK[2];
extern Bitboard PAWN_ADVANCE_MASK_1[64][2];
extern Bitboard PAWN_ADVANCE_MASK_2[64][2];
extern Bitboard PAWN_CAPTURE_MASK[64][2];
extern Bitboard BETWEEN_MASK[64][64];
extern Bitboard FROMTO_MASK[64][64];
extern Bitboard PASSED_PAWN_MASK[64][2];
extern Bitboard FRONT_MASK[64][2];
extern Bitboard ADJACENT_MASK[64];
extern Bitboard CENTER_MASK;
extern Bitboard FRONT_KING_RANK1[2][64];
extern Bitboard FRONT_KING_RANK2[2][64];
extern Bitboard DISTANCE_RING[64][8];

extern uint8_t timer_count;

const uint8_t can_king_castle_mask[2] = {1, 4};
const uint8_t can_queen_castle_mask[2] = {2, 8};

const Bitboard MASK_ISOLATED[8] = {
    0x0202020202020202, 0x0505050505050505, 0x0A0A0A0A0A0A0A0A, 0x1414141414141414,
    0x2828282828282828, 0x5050505050505050, 0xA0A0A0A0A0A0A0A0, 0x4040404040404040
};
const Bitboard CENTER_FILE = 0x3C3C3C3C3C3C3C3C;
const Bitboard FILE_MASK[8] = {
    0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808,
    0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080
};

const Bitboard FILE_ABB = 0x0101010101010101ULL;
const Bitboard FILE_BBB = FILE_ABB << 1;
const Bitboard FILE_CBB = FILE_ABB << 2;
const Bitboard FILE_DBB = FILE_ABB << 3;
const Bitboard FILE_EBB = FILE_ABB << 4;
const Bitboard FILE_FBB = FILE_ABB << 5;
const Bitboard FILE_GBB = FILE_ABB << 6;
const Bitboard FILE_HBB = FILE_ABB << 7;

const Bitboard COLOR_MASKS[2] = {0xAA55AA55AA55AA55, 0x55AA55AA55AA55AA};
const int TILE_COLOR[64] = {
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0
};

extern Square ENPASSANT_INDEX[64];
extern uint64_t castlingHash[16];

extern uint8_t CASTLING_RIGHTS[64];
extern uint8_t ROOK_MOVES_CASTLE_FROM[64];
extern uint8_t ROOK_MOVES_CASTLE_TO[64];
extern uint8_t ROOK_MOVES_CASTLE_PIECE[64];

void init();

const uint8_t FLAG_EXACT = 0;
const uint8_t FLAG_BETA = 1;
const uint8_t FLAG_ALPHA = 2;

const Bitboard RANK_1BB     = 0x00000000000000FF;
const Bitboard RANK_2BB     = 0x000000000000FF00;
const Bitboard RANK_3BB     = 0x0000000000FF0000;
const Bitboard RANK_4BB     = 0x00000000FF000000;
const Bitboard RANK_5BB     = 0x000000FF00000000;
const Bitboard RANK_6BB     = 0x0000FF0000000000;
const Bitboard RANK_7BB     = 0x00FF000000000000;
const Bitboard RANK_8BB     = 0xFF00000000000000;

const uint64_t cross_lt[15] = {
    0x0000000000000001, 0x0000000000000102, 0x0000000000010204, 0x0000000001020408,
    0x0000000102040810, 0x0000010204081020, 0x0810204080000000, 0x0001020408102040,
    0x0102040810204080, 0x0204081020408000, 0x0408102040800000, 0x1020408000000000,
    0x2040800000000000, 0x4080000000000000, 0x8000000000000000
};

const uint64_t cross_rt[15] = {
    0x0100000000000000, 0x0201000000000000, 0x0402010000000000, 0x0804020100000000,
    0x1008040201000000, 0x2010080402010000, 0x4020100804020100, 0x8040201008040201,
    0x0080402010080402, 0x0000804020100804, 0x0000008040201008, 0x0000000080402010,
    0x0000000000804020, 0x0000000000008040, 0x0000000000000080
};

const uint64_t line_vertical = 0x0101010101010101;
const uint64_t line_horizontal = 0x00000000000000FF;

const uint16_t MAX_PLY = 128;

typedef struct PV {
    Move moves[MAX_PLY + 1];
    int  size;
} PV;

extern PV pv[MAX_PLY + 1];
extern PV main_pv;
extern PV debug_pv;

void get_ready();

const uint64_t _knight_targets = 0x0000005088008850;
const uint64_t _king_targets  = 0x0000000070507000;
const uint64_t _rook_vertical = 0x0101010101010101;
const uint64_t _rook_horizontal = 0xFF;

const uint64_t _pawn_line_white = 0x000000000000FF00;
const uint64_t _pawn_line_black = 0x00FF000000000000;

const int
    KNOWN_WIN = 10000,
    MATE = 32000,
    INFINITE = 32001,
    UNDEFINED = 32002,
    TIMEOUT = 32003,

    MATE_IN_MAX_PLY = MATE - MAX_PLY,
    MATED_IN_MAX_PLY = -MATE + MAX_PLY;

const int RANK_1 = 0,
          RANK_2 = 1,
          RANK_3 = 2,
          RANK_4 = 3,
          RANK_5 = 4,
          RANK_6 = 5,
          RANK_7 = 6,
          RANK_8 = 7;

const int FILE_A = 0,
          FILE_B = 1,
          FILE_C = 2,
          FILE_D = 3,
          FILE_E = 4,
          FILE_F = 5,
          FILE_G = 6,
          FILE_H = 7;

enum MoveType{
    NORMAL      = 0,
    PROMOTION_N = 0,
    PROMOTION_B = 4,
    PROMOTION_Q = 12,
    PROMOTION_R = 8,
    CASTLING    = 3,
    ENPASSANT   = 1, // Enpassant capture
    PROMOTION   = 2
};

const Move null_move = Move(~0);
const Move no_move = Move(0);

enum MoveGenType {
    SILENT = 0,
    CAPTURE = 1,
    ALL = 2
};

enum _Piece {
    no_piece = 0,
    white_occupy = 0,
    black_occupy = 1,
    white_pawn = 2,
    black_pawn = 3,
    white_knight = 4,
    black_knight = 5,
    white_bishop = 6,
    black_bishop = 7,
    white_rook = 8,
    black_rook = 9,
    white_queen = 10,
    black_queen = 11,
    white_king = 12,
    black_king = 13
};

enum _PieceType {
    NO_PIECE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

enum _Square : Square {
    A1 = 0,
        B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

// Give kings a value for SEE
const int piece_values[14] = {0, 0, PAWN_MID, PAWN_MID, KNIGHT_MID, KNIGHT_MID, BISHOP_MID, BISHOP_MID,
                              ROOK_MID, ROOK_MID, QUEEN_MID, QUEEN_MID, 100 * QUEEN_MID, 100 * QUEEN_MID};

const char piece_chars[14] = {'\0', '\0', '\0', '\0', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};
extern int mvvlva_values[12][14];

extern Score pst[14][64];

typedef struct CopyThingSize {
    //? COPIED 
    uint64_t     pawn_hash;
    Piece        pieces[64];
    Bitboard     bbs[14];
    Score        score;
    uint8_t      castling; // black_queenside | black_kingside | white_queenside | white_kingside
    Square       king_index[2];
    Bitboard     board;
    uint8_t      last_irreversible;
    int          material_index;
    int          non_pawn_material[2];
    SearchThread *my_thread;
} CopyThingSize;

typedef struct Metadata {
    int  ply;
    Move current_move;
    int  static_eval;
    Move killers[2];
    Move excluded_move;
} Metadata;

struct Position {
    //? COPIED 
    uint64_t     pawn_hash;
    Piece        pieces[64];
    Bitboard     bbs[14];
    Score        score;
    uint8_t      castling; // black_queenside | black_kingside | white_queenside | white_kingside
    Square       king_index[2];
    Bitboard     board;
    uint8_t      last_irreversible;
    int          material_index;
    int          non_pawn_material[2];
    SearchThread *my_thread;

    //! NOT TO COPY
    Square   enpassant; 
    Color    color;
    uint64_t hash;
    Bitboard pinned[2];
};

const int position_size = sizeof(CopyThingSize);

typedef struct Evaluation {
    // Position *position;
    Bitboard targets[14];
    Score    score;
    Score    score_pawn;
    Score    score_white;
    Score    score_black;
    int      num_pieces[14];
    int      num_king_attackers[2];
    int      num_king_zone_attacks[2];
    int      king_zone_score[2];
    Bitboard double_targets[2];
    Bitboard king_zone[2];
    int      semi_open_files[2];
    Bitboard mobility_area[2];
    Score    mobility_score[2];
    Bitboard pawn_passers[2];
    Square   bishop_squares[2];
} Evaluation;

const Evaluation init_evaluation = Evaluation{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

enum EndgameType {
    NORMAL_ENDGAME,
    DRAW_ENDGAME,
    KNOWN_ENDGAME_KPK,
    KNOWN_ENDGAME_KXK
};

enum ScalingFactorType {
    NO_SCALING,
    KRPKR_SCALING
};

const int SCALE_NORMAL = 32;

typedef struct Material {
    int               phase;
    int               score;
    EndgameType       endgame_type;
    ScalingFactorType scaling_factor_type;
} Material;

extern Material material_base[9*3*3*3*2*9*3*3*3*2];

const int material_balance[14] ={
    0, 0,
    2*2*3*3*3*3*9,   // White Pawn
    2*2*3*3*3*3*9*9, // Black Pawn
    2*2*3*3*3*3,     // White Knight
    2*2*3*3*3*3*3,   // Black Knight
    2*2*3*3,         // White Bishop
    2*2*3*3*3,       // Black Bishop
    2*2,             // White Rook
    2*2*3,           // Black Rook
    1,               // White Queen
    1*2,             // Black Queen
    0, 0,            // Kings
};

inline Material *get_material(Position *p) { return &material_base[p->material_index]; }

enum SearchType {
    NORMAL_SEARCH = 0,
    QUIESCENCE_SEARCH = 1,
    PERFT_SEARCH = 2
};

extern struct timeval curr_time, start_ts;

inline int time_passed() {
    return (((curr_time.tv_sec - start_ts.tv_sec) * 1000000) + (curr_time.tv_usec - start_ts.tv_usec)) / 1000;
}

typedef struct ScoredMove {
    Move move;
    int  score;
} ScoredMove;

bool scored_move_compare(ScoredMove lhs, ScoredMove rhs);
bool scored_move_compare_greater(ScoredMove lhs, ScoredMove rhs);

const int
    // Regular stages
    NORMAL_TTE_MOVE = 0,

    GOOD_CAPTURES_SORT = 1,
    GOOD_CAPTURES = 2,

    KILLER_MOVES = 3,
    COUNTER_MOVES = 4,

    QUIETS_SORT = 5,
    QUIETS = 6,

    BAD_CAPTURES = 7,

    // Evasion stages
    EVASION_TTE_MOVE = 8,
    EVASIONS_SORT = 9,
    EVASIONS = 10,

    // Quiescence stages
    QUIESCENCE_TTE_MOVE = 11,
    QUIESCENCE_CAPTURES_SORT = 12,
    QUIESCENCE_CAPTURES = 13,

    // Quiescence stages
    QUIESCENCE_TTE_MOVE_CHECKS = 14,
    QUIESCENCE_CAPTURES_SORT_CHECKS = 15,
    QUIESCENCE_CAPTURES_CHECKS = 16,
    QUIESCENCE_QUIETS_CHECKS = 17;

enum ScoreType {
    SCORE_CAPTURE = 0,
    SCORE_QUIET = 1,
    SCORE_EVASION = 2
};

struct MoveGen {
    ScoredMove moves[256];
    Position   *position;
    Move       tte_move;
    Move       killer_moves[2];
    Move       counter_move;
    int        stage;
    uint8_t    head;
    uint8_t    tail;
    int        end_bad_captures;
    int        depth;
};

const MoveGen blank_movegen = {
    {}, // Moves
    0, // Position
    0, // tte_move
    {0, 0}, // killers
    0, // counter move
    0, // stage
    0, // head
    0, // tail
    0, // end bad captures
    0 // ply
};

struct SearchThread {
    std::thread thread_obj;
    Position    positions[1024];
    int         thread_id;
    int         search_ply;
    Metadata    metadatas[MAX_PLY + 1];
    Move        counter_moves[14][64];
    int         history[14][64];
    int         depth;
    uint64_t    nodes;
    uint64_t    tb_hits;
};

inline bool is_main_thread(Position *p) {return p->my_thread->thread_id == 0;}

const int MAX_THREADS = 64;
extern SearchThread search_threads[MAX_THREADS];

extern int myremain;
extern int total_remaining;
extern int moves_to_go;
extern volatile bool is_timeout;
extern int root_ply;
extern int think_depth_limit;
extern int num_threads;
extern int move_overhead;

inline void initialize_threads() {
    for (int i = 0; i < num_threads; ++i) {
        search_threads[i].nodes = 0;
        search_threads[i].tb_hits = 0;
    }
}

inline uint64_t sum_nodes() {
    uint64_t s = 0;
    for (int i = 0; i < num_threads; ++i) {
        s += search_threads[i].nodes;
    }
    return s;
}

inline uint64_t sum_tb_hits() {
    uint64_t s = 0;
    for (int i = 0; i < num_threads; ++i) {
        s += search_threads[i].tb_hits;
    }
    return s;
}

extern int reductions[2][64][64];

inline Color piece_color(Piece p) {return p & 1;}

inline bool is_white(Piece p) {return piece_color(p) == white;}

inline bool is_black(Piece p) {return piece_color(p) == black;}

inline int piece_type(Piece p) {return int(p / 2);}

inline Piece king(Color color) {return white_king | color;}

inline Piece queen(Color color) {return white_queen | color;}

inline Piece rook(Color color) {return white_rook | color;}

inline Piece bishop(Color color) {return white_bishop | color;}

inline Piece knight(Color color) {return white_knight | color;}

inline Piece pawn(Color color) {return white_pawn | color;}

inline Piece occupy(Color color) {return white_occupy | color;}

inline bool is_king(Piece p) {return piece_type(p) == KING;}

inline bool is_queen(Piece p) {return piece_type(p) == QUEEN;}

inline bool is_rook(Piece p) {return piece_type(p) == ROOK;}

inline bool is_bishop(Piece p) {return piece_type(p) == BISHOP;}

inline bool is_knight(Piece p) {return piece_type(p) == KNIGHT;}

inline bool is_pawn(Piece p) {return piece_type(p) == PAWN;}

inline int move_from(Move m) {return m >> 10;}

inline int move_to(Move m) {return (m >> 4) & 63;}

inline bool is_move_empty(Move m) {return move_from(m) == move_to(m);}

inline int row(Square s) {return s >> 3;}

inline int col(Square s) {return s & 7;}

inline int rank(Square s, Color color) {return row(s) ^ (color * 7);}

inline int distance(Square s1, Square s2) {
    int col_distance = std::abs(col(s1) - col(s2));
    int row_distance = std::abs(row(s1) - row(s2));
    return std::max(col_distance, row_distance);
}

inline Square mirror_square(Square s, Color color) {return color == white ? s : s ^ H8;}

inline Square relative_square(Square s, Color color) {return color == white ? s : s ^ A8;}

inline Square pawn_forward(Square s, Color color) {return color == white ? s + 8 : s - 8;}

inline int move_type(Move m) {return m & 3;}

// inline bool is_castling(Move m_type) {return (m_type == CASTLING);}

// inline bool is_capture(Move m_type) {return (m_type & CAPTURE);}

inline bool is_promotion(Move m_type) {return (m_type == PROMOTION);}

// inline bool is_enpassant(Move m_type) {return (m_type == ENPASSANT);}

// inline bool is_enpassant_capture(Move m_type) {return (m_type == ENPASSANT_C);}

// inline bool is_quiet(Move m_type) {return (m_type & 0b1100) == 0;}

inline bool is_queen_promotion(Move m){return (m & 12) == PROMOTION_Q;}

inline bool is_rook_promotion(Move m){return (m & 12) == PROMOTION_R;}

inline bool is_bishop_promotion(Move m){return (m & 12) == PROMOTION_B;}

inline bool is_knight_promotion(Move m){return (m & 12) == PROMOTION_N;}

inline Color opponent_color(Color c) {return c ^ 1;}

inline bool opposite_colors(Square s1, Square s2) {
    int s = s1 ^ s2;
    return ((s >> 3) ^ s) & 1;
}

inline Piece promotion_type(Move m) {return m & 12;}

inline Piece promotion_piece(Move m, Color c){
    uint8_t promotion = m & 12;
    if (promotion == 0)
        return white_knight + c;
    else if (promotion == 4)
        return white_bishop + c;
    else if (promotion == 8)
        return white_rook + c;
    else
        return white_queen + c;
}

extern uint64_t polyglotCombined[14][64];

const int polyglotPieces[14] = {
    0,
    0,
    64,
    0,
    192,
    128,
    320,
    256,
    448,
    384,
    576,
    512,
    704,
    640
};

const uint64_t polyglotArray[781] = {
   0x9D39247E33776D41ULL, 0x2AF7398005AAA5C7ULL, 0x44DB015024623547ULL, 0x9C15F73E62A76AE2ULL,
   0x75834465489C0C89ULL, 0x3290AC3A203001BFULL, 0x0FBBAD1F61042279ULL, 0xE83A908FF2FB60CAULL,
   0x0D7E765D58755C10ULL, 0x1A083822CEAFE02DULL, 0x9605D5F0E25EC3B0ULL, 0xD021FF5CD13A2ED5ULL,
   0x40BDF15D4A672E32ULL, 0x011355146FD56395ULL, 0x5DB4832046F3D9E5ULL, 0x239F8B2D7FF719CCULL,
   0x05D1A1AE85B49AA1ULL, 0x679F848F6E8FC971ULL, 0x7449BBFF801FED0BULL, 0x7D11CDB1C3B7ADF0ULL,
   0x82C7709E781EB7CCULL, 0xF3218F1C9510786CULL, 0x331478F3AF51BBE6ULL, 0x4BB38DE5E7219443ULL,
   0xAA649C6EBCFD50FCULL, 0x8DBD98A352AFD40BULL, 0x87D2074B81D79217ULL, 0x19F3C751D3E92AE1ULL,
   0xB4AB30F062B19ABFULL, 0x7B0500AC42047AC4ULL, 0xC9452CA81A09D85DULL, 0x24AA6C514DA27500ULL,
   0x4C9F34427501B447ULL, 0x14A68FD73C910841ULL, 0xA71B9B83461CBD93ULL, 0x03488B95B0F1850FULL,
   0x637B2B34FF93C040ULL, 0x09D1BC9A3DD90A94ULL, 0x3575668334A1DD3BULL, 0x735E2B97A4C45A23ULL,
   0x18727070F1BD400BULL, 0x1FCBACD259BF02E7ULL, 0xD310A7C2CE9B6555ULL, 0xBF983FE0FE5D8244ULL,
   0x9F74D14F7454A824ULL, 0x51EBDC4AB9BA3035ULL, 0x5C82C505DB9AB0FAULL, 0xFCF7FE8A3430B241ULL,
   0x3253A729B9BA3DDEULL, 0x8C74C368081B3075ULL, 0xB9BC6C87167C33E7ULL, 0x7EF48F2B83024E20ULL,
   0x11D505D4C351BD7FULL, 0x6568FCA92C76A243ULL, 0x4DE0B0F40F32A7B8ULL, 0x96D693460CC37E5DULL,
   0x42E240CB63689F2FULL, 0x6D2BDCDAE2919661ULL, 0x42880B0236E4D951ULL, 0x5F0F4A5898171BB6ULL,
   0x39F890F579F92F88ULL, 0x93C5B5F47356388BULL, 0x63DC359D8D231B78ULL, 0xEC16CA8AEA98AD76ULL,
   0x5355F900C2A82DC7ULL, 0x07FB9F855A997142ULL, 0x5093417AA8A7ED5EULL, 0x7BCBC38DA25A7F3CULL,
   0x19FC8A768CF4B6D4ULL, 0x637A7780DECFC0D9ULL, 0x8249A47AEE0E41F7ULL, 0x79AD695501E7D1E8ULL,
   0x14ACBAF4777D5776ULL, 0xF145B6BECCDEA195ULL, 0xDABF2AC8201752FCULL, 0x24C3C94DF9C8D3F6ULL,
   0xBB6E2924F03912EAULL, 0x0CE26C0B95C980D9ULL, 0xA49CD132BFBF7CC4ULL, 0xE99D662AF4243939ULL,
   0x27E6AD7891165C3FULL, 0x8535F040B9744FF1ULL, 0x54B3F4FA5F40D873ULL, 0x72B12C32127FED2BULL,
   0xEE954D3C7B411F47ULL, 0x9A85AC909A24EAA1ULL, 0x70AC4CD9F04F21F5ULL, 0xF9B89D3E99A075C2ULL,
   0x87B3E2B2B5C907B1ULL, 0xA366E5B8C54F48B8ULL, 0xAE4A9346CC3F7CF2ULL, 0x1920C04D47267BBDULL,
   0x87BF02C6B49E2AE9ULL, 0x092237AC237F3859ULL, 0xFF07F64EF8ED14D0ULL, 0x8DE8DCA9F03CC54EULL,
   0x9C1633264DB49C89ULL, 0xB3F22C3D0B0B38EDULL, 0x390E5FB44D01144BULL, 0x5BFEA5B4712768E9ULL,
   0x1E1032911FA78984ULL, 0x9A74ACB964E78CB3ULL, 0x4F80F7A035DAFB04ULL, 0x6304D09A0B3738C4ULL,
   0x2171E64683023A08ULL, 0x5B9B63EB9CEFF80CULL, 0x506AACF489889342ULL, 0x1881AFC9A3A701D6ULL,
   0x6503080440750644ULL, 0xDFD395339CDBF4A7ULL, 0xEF927DBCF00C20F2ULL, 0x7B32F7D1E03680ECULL,
   0xB9FD7620E7316243ULL, 0x05A7E8A57DB91B77ULL, 0xB5889C6E15630A75ULL, 0x4A750A09CE9573F7ULL,
   0xCF464CEC899A2F8AULL, 0xF538639CE705B824ULL, 0x3C79A0FF5580EF7FULL, 0xEDE6C87F8477609DULL,
   0x799E81F05BC93F31ULL, 0x86536B8CF3428A8CULL, 0x97D7374C60087B73ULL, 0xA246637CFF328532ULL,
   0x043FCAE60CC0EBA0ULL, 0x920E449535DD359EULL, 0x70EB093B15B290CCULL, 0x73A1921916591CBDULL,
   0x56436C9FE1A1AA8DULL, 0xEFAC4B70633B8F81ULL, 0xBB215798D45DF7AFULL, 0x45F20042F24F1768ULL,
   0x930F80F4E8EB7462ULL, 0xFF6712FFCFD75EA1ULL, 0xAE623FD67468AA70ULL, 0xDD2C5BC84BC8D8FCULL,
   0x7EED120D54CF2DD9ULL, 0x22FE545401165F1CULL, 0xC91800E98FB99929ULL, 0x808BD68E6AC10365ULL,
   0xDEC468145B7605F6ULL, 0x1BEDE3A3AEF53302ULL, 0x43539603D6C55602ULL, 0xAA969B5C691CCB7AULL,
   0xA87832D392EFEE56ULL, 0x65942C7B3C7E11AEULL, 0xDED2D633CAD004F6ULL, 0x21F08570F420E565ULL,
   0xB415938D7DA94E3CULL, 0x91B859E59ECB6350ULL, 0x10CFF333E0ED804AULL, 0x28AED140BE0BB7DDULL,
   0xC5CC1D89724FA456ULL, 0x5648F680F11A2741ULL, 0x2D255069F0B7DAB3ULL, 0x9BC5A38EF729ABD4ULL,
   0xEF2F054308F6A2BCULL, 0xAF2042F5CC5C2858ULL, 0x480412BAB7F5BE2AULL, 0xAEF3AF4A563DFE43ULL,
   0x19AFE59AE451497FULL, 0x52593803DFF1E840ULL, 0xF4F076E65F2CE6F0ULL, 0x11379625747D5AF3ULL,
   0xBCE5D2248682C115ULL, 0x9DA4243DE836994FULL, 0x066F70B33FE09017ULL, 0x4DC4DE189B671A1CULL,
   0x51039AB7712457C3ULL, 0xC07A3F80C31FB4B4ULL, 0xB46EE9C5E64A6E7CULL, 0xB3819A42ABE61C87ULL,
   0x21A007933A522A20ULL, 0x2DF16F761598AA4FULL, 0x763C4A1371B368FDULL, 0xF793C46702E086A0ULL,
   0xD7288E012AEB8D31ULL, 0xDE336A2A4BC1C44BULL, 0x0BF692B38D079F23ULL, 0x2C604A7A177326B3ULL,
   0x4850E73E03EB6064ULL, 0xCFC447F1E53C8E1BULL, 0xB05CA3F564268D99ULL, 0x9AE182C8BC9474E8ULL,
   0xA4FC4BD4FC5558CAULL, 0xE755178D58FC4E76ULL, 0x69B97DB1A4C03DFEULL, 0xF9B5B7C4ACC67C96ULL,
   0xFC6A82D64B8655FBULL, 0x9C684CB6C4D24417ULL, 0x8EC97D2917456ED0ULL, 0x6703DF9D2924E97EULL,
   0xC547F57E42A7444EULL, 0x78E37644E7CAD29EULL, 0xFE9A44E9362F05FAULL, 0x08BD35CC38336615ULL,
   0x9315E5EB3A129ACEULL, 0x94061B871E04DF75ULL, 0xDF1D9F9D784BA010ULL, 0x3BBA57B68871B59DULL,
   0xD2B7ADEEDED1F73FULL, 0xF7A255D83BC373F8ULL, 0xD7F4F2448C0CEB81ULL, 0xD95BE88CD210FFA7ULL,
   0x336F52F8FF4728E7ULL, 0xA74049DAC312AC71ULL, 0xA2F61BB6E437FDB5ULL, 0x4F2A5CB07F6A35B3ULL,
   0x87D380BDA5BF7859ULL, 0x16B9F7E06C453A21ULL, 0x7BA2484C8A0FD54EULL, 0xF3A678CAD9A2E38CULL,
   0x39B0BF7DDE437BA2ULL, 0xFCAF55C1BF8A4424ULL, 0x18FCF680573FA594ULL, 0x4C0563B89F495AC3ULL,
   0x40E087931A00930DULL, 0x8CFFA9412EB642C1ULL, 0x68CA39053261169FULL, 0x7A1EE967D27579E2ULL,
   0x9D1D60E5076F5B6FULL, 0x3810E399B6F65BA2ULL, 0x32095B6D4AB5F9B1ULL, 0x35CAB62109DD038AULL,
   0xA90B24499FCFAFB1ULL, 0x77A225A07CC2C6BDULL, 0x513E5E634C70E331ULL, 0x4361C0CA3F692F12ULL,
   0xD941ACA44B20A45BULL, 0x528F7C8602C5807BULL, 0x52AB92BEB9613989ULL, 0x9D1DFA2EFC557F73ULL,
   0x722FF175F572C348ULL, 0x1D1260A51107FE97ULL, 0x7A249A57EC0C9BA2ULL, 0x04208FE9E8F7F2D6ULL,
   0x5A110C6058B920A0ULL, 0x0CD9A497658A5698ULL, 0x56FD23C8F9715A4CULL, 0x284C847B9D887AAEULL,
   0x04FEABFBBDB619CBULL, 0x742E1E651C60BA83ULL, 0x9A9632E65904AD3CULL, 0x881B82A13B51B9E2ULL,
   0x506E6744CD974924ULL, 0xB0183DB56FFC6A79ULL, 0x0ED9B915C66ED37EULL, 0x5E11E86D5873D484ULL,
   0xF678647E3519AC6EULL, 0x1B85D488D0F20CC5ULL, 0xDAB9FE6525D89021ULL, 0x0D151D86ADB73615ULL,
   0xA865A54EDCC0F019ULL, 0x93C42566AEF98FFBULL, 0x99E7AFEABE000731ULL, 0x48CBFF086DDF285AULL,
   0x7F9B6AF1EBF78BAFULL, 0x58627E1A149BBA21ULL, 0x2CD16E2ABD791E33ULL, 0xD363EFF5F0977996ULL,
   0x0CE2A38C344A6EEDULL, 0x1A804AADB9CFA741ULL, 0x907F30421D78C5DEULL, 0x501F65EDB3034D07ULL,
   0x37624AE5A48FA6E9ULL, 0x957BAF61700CFF4EULL, 0x3A6C27934E31188AULL, 0xD49503536ABCA345ULL,
   0x088E049589C432E0ULL, 0xF943AEE7FEBF21B8ULL, 0x6C3B8E3E336139D3ULL, 0x364F6FFA464EE52EULL,
   0xD60F6DCEDC314222ULL, 0x56963B0DCA418FC0ULL, 0x16F50EDF91E513AFULL, 0xEF1955914B609F93ULL,
   0x565601C0364E3228ULL, 0xECB53939887E8175ULL, 0xBAC7A9A18531294BULL, 0xB344C470397BBA52ULL,
   0x65D34954DAF3CEBDULL, 0xB4B81B3FA97511E2ULL, 0xB422061193D6F6A7ULL, 0x071582401C38434DULL,
   0x7A13F18BBEDC4FF5ULL, 0xBC4097B116C524D2ULL, 0x59B97885E2F2EA28ULL, 0x99170A5DC3115544ULL,
   0x6F423357E7C6A9F9ULL, 0x325928EE6E6F8794ULL, 0xD0E4366228B03343ULL, 0x565C31F7DE89EA27ULL,
   0x30F5611484119414ULL, 0xD873DB391292ED4FULL, 0x7BD94E1D8E17DEBCULL, 0xC7D9F16864A76E94ULL,
   0x947AE053EE56E63CULL, 0xC8C93882F9475F5FULL, 0x3A9BF55BA91F81CAULL, 0xD9A11FBB3D9808E4ULL,
   0x0FD22063EDC29FCAULL, 0xB3F256D8ACA0B0B9ULL, 0xB03031A8B4516E84ULL, 0x35DD37D5871448AFULL,
   0xE9F6082B05542E4EULL, 0xEBFAFA33D7254B59ULL, 0x9255ABB50D532280ULL, 0xB9AB4CE57F2D34F3ULL,
   0x693501D628297551ULL, 0xC62C58F97DD949BFULL, 0xCD454F8F19C5126AULL, 0xBBE83F4ECC2BDECBULL,
   0xDC842B7E2819E230ULL, 0xBA89142E007503B8ULL, 0xA3BC941D0A5061CBULL, 0xE9F6760E32CD8021ULL,
   0x09C7E552BC76492FULL, 0x852F54934DA55CC9ULL, 0x8107FCCF064FCF56ULL, 0x098954D51FFF6580ULL,
   0x23B70EDB1955C4BFULL, 0xC330DE426430F69DULL, 0x4715ED43E8A45C0AULL, 0xA8D7E4DAB780A08DULL,
   0x0572B974F03CE0BBULL, 0xB57D2E985E1419C7ULL, 0xE8D9ECBE2CF3D73FULL, 0x2FE4B17170E59750ULL,
   0x11317BA87905E790ULL, 0x7FBF21EC8A1F45ECULL, 0x1725CABFCB045B00ULL, 0x964E915CD5E2B207ULL,
   0x3E2B8BCBF016D66DULL, 0xBE7444E39328A0ACULL, 0xF85B2B4FBCDE44B7ULL, 0x49353FEA39BA63B1ULL,
   0x1DD01AAFCD53486AULL, 0x1FCA8A92FD719F85ULL, 0xFC7C95D827357AFAULL, 0x18A6A990C8B35EBDULL,
   0xCCCB7005C6B9C28DULL, 0x3BDBB92C43B17F26ULL, 0xAA70B5B4F89695A2ULL, 0xE94C39A54A98307FULL,
   0xB7A0B174CFF6F36EULL, 0xD4DBA84729AF48ADULL, 0x2E18BC1AD9704A68ULL, 0x2DE0966DAF2F8B1CULL,
   0xB9C11D5B1E43A07EULL, 0x64972D68DEE33360ULL, 0x94628D38D0C20584ULL, 0xDBC0D2B6AB90A559ULL,
   0xD2733C4335C6A72FULL, 0x7E75D99D94A70F4DULL, 0x6CED1983376FA72BULL, 0x97FCAACBF030BC24ULL,
   0x7B77497B32503B12ULL, 0x8547EDDFB81CCB94ULL, 0x79999CDFF70902CBULL, 0xCFFE1939438E9B24ULL,
   0x829626E3892D95D7ULL, 0x92FAE24291F2B3F1ULL, 0x63E22C147B9C3403ULL, 0xC678B6D860284A1CULL,
   0x5873888850659AE7ULL, 0x0981DCD296A8736DULL, 0x9F65789A6509A440ULL, 0x9FF38FED72E9052FULL,
   0xE479EE5B9930578CULL, 0xE7F28ECD2D49EECDULL, 0x56C074A581EA17FEULL, 0x5544F7D774B14AEFULL,
   0x7B3F0195FC6F290FULL, 0x12153635B2C0CF57ULL, 0x7F5126DBBA5E0CA7ULL, 0x7A76956C3EAFB413ULL,
   0x3D5774A11D31AB39ULL, 0x8A1B083821F40CB4ULL, 0x7B4A38E32537DF62ULL, 0x950113646D1D6E03ULL,
   0x4DA8979A0041E8A9ULL, 0x3BC36E078F7515D7ULL, 0x5D0A12F27AD310D1ULL, 0x7F9D1A2E1EBE1327ULL,
   0xDA3A361B1C5157B1ULL, 0xDCDD7D20903D0C25ULL, 0x36833336D068F707ULL, 0xCE68341F79893389ULL,
   0xAB9090168DD05F34ULL, 0x43954B3252DC25E5ULL, 0xB438C2B67F98E5E9ULL, 0x10DCD78E3851A492ULL,
   0xDBC27AB5447822BFULL, 0x9B3CDB65F82CA382ULL, 0xB67B7896167B4C84ULL, 0xBFCED1B0048EAC50ULL,
   0xA9119B60369FFEBDULL, 0x1FFF7AC80904BF45ULL, 0xAC12FB171817EEE7ULL, 0xAF08DA9177DDA93DULL,
   0x1B0CAB936E65C744ULL, 0xB559EB1D04E5E932ULL, 0xC37B45B3F8D6F2BAULL, 0xC3A9DC228CAAC9E9ULL,
   0xF3B8B6675A6507FFULL, 0x9FC477DE4ED681DAULL, 0x67378D8ECCEF96CBULL, 0x6DD856D94D259236ULL,
   0xA319CE15B0B4DB31ULL, 0x073973751F12DD5EULL, 0x8A8E849EB32781A5ULL, 0xE1925C71285279F5ULL,
   0x74C04BF1790C0EFEULL, 0x4DDA48153C94938AULL, 0x9D266D6A1CC0542CULL, 0x7440FB816508C4FEULL,
   0x13328503DF48229FULL, 0xD6BF7BAEE43CAC40ULL, 0x4838D65F6EF6748FULL, 0x1E152328F3318DEAULL,
   0x8F8419A348F296BFULL, 0x72C8834A5957B511ULL, 0xD7A023A73260B45CULL, 0x94EBC8ABCFB56DAEULL,
   0x9FC10D0F989993E0ULL, 0xDE68A2355B93CAE6ULL, 0xA44CFE79AE538BBEULL, 0x9D1D84FCCE371425ULL,
   0x51D2B1AB2DDFB636ULL, 0x2FD7E4B9E72CD38CULL, 0x65CA5B96B7552210ULL, 0xDD69A0D8AB3B546DULL,
   0x604D51B25FBF70E2ULL, 0x73AA8A564FB7AC9EULL, 0x1A8C1E992B941148ULL, 0xAAC40A2703D9BEA0ULL,
   0x764DBEAE7FA4F3A6ULL, 0x1E99B96E70A9BE8BULL, 0x2C5E9DEB57EF4743ULL, 0x3A938FEE32D29981ULL,
   0x26E6DB8FFDF5ADFEULL, 0x469356C504EC9F9DULL, 0xC8763C5B08D1908CULL, 0x3F6C6AF859D80055ULL,
   0x7F7CC39420A3A545ULL, 0x9BFB227EBDF4C5CEULL, 0x89039D79D6FC5C5CULL, 0x8FE88B57305E2AB6ULL,
   0xA09E8C8C35AB96DEULL, 0xFA7E393983325753ULL, 0xD6B6D0ECC617C699ULL, 0xDFEA21EA9E7557E3ULL,
   0xB67C1FA481680AF8ULL, 0xCA1E3785A9E724E5ULL, 0x1CFC8BED0D681639ULL, 0xD18D8549D140CAEAULL,
   0x4ED0FE7E9DC91335ULL, 0xE4DBF0634473F5D2ULL, 0x1761F93A44D5AEFEULL, 0x53898E4C3910DA55ULL,
   0x734DE8181F6EC39AULL, 0x2680B122BAA28D97ULL, 0x298AF231C85BAFABULL, 0x7983EED3740847D5ULL,
   0x66C1A2A1A60CD889ULL, 0x9E17E49642A3E4C1ULL, 0xEDB454E7BADC0805ULL, 0x50B704CAB602C329ULL,
   0x4CC317FB9CDDD023ULL, 0x66B4835D9EAFEA22ULL, 0x219B97E26FFC81BDULL, 0x261E4E4C0A333A9DULL,
   0x1FE2CCA76517DB90ULL, 0xD7504DFA8816EDBBULL, 0xB9571FA04DC089C8ULL, 0x1DDC0325259B27DEULL,
   0xCF3F4688801EB9AAULL, 0xF4F5D05C10CAB243ULL, 0x38B6525C21A42B0EULL, 0x36F60E2BA4FA6800ULL,
   0xEB3593803173E0CEULL, 0x9C4CD6257C5A3603ULL, 0xAF0C317D32ADAA8AULL, 0x258E5A80C7204C4BULL,
   0x8B889D624D44885DULL, 0xF4D14597E660F855ULL, 0xD4347F66EC8941C3ULL, 0xE699ED85B0DFB40DULL,
   0x2472F6207C2D0484ULL, 0xC2A1E7B5B459AEB5ULL, 0xAB4F6451CC1D45ECULL, 0x63767572AE3D6174ULL,
   0xA59E0BD101731A28ULL, 0x116D0016CB948F09ULL, 0x2CF9C8CA052F6E9FULL, 0x0B090A7560A968E3ULL,
   0xABEEDDB2DDE06FF1ULL, 0x58EFC10B06A2068DULL, 0xC6E57A78FBD986E0ULL, 0x2EAB8CA63CE802D7ULL,
   0x14A195640116F336ULL, 0x7C0828DD624EC390ULL, 0xD74BBE77E6116AC7ULL, 0x804456AF10F5FB53ULL,
   0xEBE9EA2ADF4321C7ULL, 0x03219A39EE587A30ULL, 0x49787FEF17AF9924ULL, 0xA1E9300CD8520548ULL,
   0x5B45E522E4B1B4EFULL, 0xB49C3B3995091A36ULL, 0xD4490AD526F14431ULL, 0x12A8F216AF9418C2ULL,
   0x001F837CC7350524ULL, 0x1877B51E57A764D5ULL, 0xA2853B80F17F58EEULL, 0x993E1DE72D36D310ULL,
   0xB3598080CE64A656ULL, 0x252F59CF0D9F04BBULL, 0xD23C8E176D113600ULL, 0x1BDA0492E7E4586EULL,
   0x21E0BD5026C619BFULL, 0x3B097ADAF088F94EULL, 0x8D14DEDB30BE846EULL, 0xF95CFFA23AF5F6F4ULL,
   0x3871700761B3F743ULL, 0xCA672B91E9E4FA16ULL, 0x64C8E531BFF53B55ULL, 0x241260ED4AD1E87DULL,
   0x106C09B972D2E822ULL, 0x7FBA195410E5CA30ULL, 0x7884D9BC6CB569D8ULL, 0x0647DFEDCD894A29ULL,
   0x63573FF03E224774ULL, 0x4FC8E9560F91B123ULL, 0x1DB956E450275779ULL, 0xB8D91274B9E9D4FBULL,
   0xA2EBEE47E2FBFCE1ULL, 0xD9F1F30CCD97FB09ULL, 0xEFED53D75FD64E6BULL, 0x2E6D02C36017F67FULL,
   0xA9AA4D20DB084E9BULL, 0xB64BE8D8B25396C1ULL, 0x70CB6AF7C2D5BCF0ULL, 0x98F076A4F7A2322EULL,
   0xBF84470805E69B5FULL, 0x94C3251F06F90CF3ULL, 0x3E003E616A6591E9ULL, 0xB925A6CD0421AFF3ULL,
   0x61BDD1307C66E300ULL, 0xBF8D5108E27E0D48ULL, 0x240AB57A8B888B20ULL, 0xFC87614BAF287E07ULL,
   0xEF02CDD06FFDB432ULL, 0xA1082C0466DF6C0AULL, 0x8215E577001332C8ULL, 0xD39BB9C3A48DB6CFULL,
   0x2738259634305C14ULL, 0x61CF4F94C97DF93DULL, 0x1B6BACA2AE4E125BULL, 0x758F450C88572E0BULL,
   0x959F587D507A8359ULL, 0xB063E962E045F54DULL, 0x60E8ED72C0DFF5D1ULL, 0x7B64978555326F9FULL,
   0xFD080D236DA814BAULL, 0x8C90FD9B083F4558ULL, 0x106F72FE81E2C590ULL, 0x7976033A39F7D952ULL,
   0xA4EC0132764CA04BULL, 0x733EA705FAE4FA77ULL, 0xB4D8F77BC3E56167ULL, 0x9E21F4F903B33FD9ULL,
   0x9D765E419FB69F6DULL, 0xD30C088BA61EA5EFULL, 0x5D94337FBFAF7F5BULL, 0x1A4E4822EB4D7A59ULL,
   0x6FFE73E81B637FB3ULL, 0xDDF957BC36D8B9CAULL, 0x64D0E29EEA8838B3ULL, 0x08DD9BDFD96B9F63ULL,
   0x087E79E5A57D1D13ULL, 0xE328E230E3E2B3FBULL, 0x1C2559E30F0946BEULL, 0x720BF5F26F4D2EAAULL,
   0xB0774D261CC609DBULL, 0x443F64EC5A371195ULL, 0x4112CF68649A260EULL, 0xD813F2FAB7F5C5CAULL,
   0x660D3257380841EEULL, 0x59AC2C7873F910A3ULL, 0xE846963877671A17ULL, 0x93B633ABFA3469F8ULL,
   0xC0C0F5A60EF4CDCFULL, 0xCAF21ECD4377B28CULL, 0x57277707199B8175ULL, 0x506C11B9D90E8B1DULL,
   0xD83CC2687A19255FULL, 0x4A29C6465A314CD1ULL, 0xED2DF21216235097ULL, 0xB5635C95FF7296E2ULL,
   0x22AF003AB672E811ULL, 0x52E762596BF68235ULL, 0x9AEBA33AC6ECC6B0ULL, 0x944F6DE09134DFB6ULL,
   0x6C47BEC883A7DE39ULL, 0x6AD047C430A12104ULL, 0xA5B1CFDBA0AB4067ULL, 0x7C45D833AFF07862ULL,
   0x5092EF950A16DA0BULL, 0x9338E69C052B8E7BULL, 0x455A4B4CFE30E3F5ULL, 0x6B02E63195AD0CF8ULL,
   0x6B17B224BAD6BF27ULL, 0xD1E0CCD25BB9C169ULL, 0xDE0C89A556B9AE70ULL, 0x50065E535A213CF6ULL,
   0x9C1169FA2777B874ULL, 0x78EDEFD694AF1EEDULL, 0x6DC93D9526A50E68ULL, 0xEE97F453F06791EDULL,
   0x32AB0EDB696703D3ULL, 0x3A6853C7E70757A7ULL, 0x31865CED6120F37DULL, 0x67FEF95D92607890ULL,
   0x1F2B1D1F15F6DC9CULL, 0xB69E38A8965C6B65ULL, 0xAA9119FF184CCCF4ULL, 0xF43C732873F24C13ULL,
   0xFB4A3D794A9A80D2ULL, 0x3550C2321FD6109CULL, 0x371F77E76BB8417EULL, 0x6BFA9AAE5EC05779ULL,
   0xCD04F3FF001A4778ULL, 0xE3273522064480CAULL, 0x9F91508BFFCFC14AULL, 0x049A7F41061A9E60ULL,
   0xFCB6BE43A9F2FE9BULL, 0x08DE8A1C7797DA9BULL, 0x8F9887E6078735A1ULL, 0xB5B4071DBFC73A66ULL,
   0x230E343DFBA08D33ULL, 0x43ED7F5A0FAE657DULL, 0x3A88A0FBBCB05C63ULL, 0x21874B8B4D2DBC4FULL,
   0x1BDEA12E35F6A8C9ULL, 0x53C065C6C8E63528ULL, 0xE34A1D250E7A8D6BULL, 0xD6B04D3B7651DD7EULL,
   0x5E90277E7CB39E2DULL, 0x2C046F22062DC67DULL, 0xB10BB459132D0A26ULL, 0x3FA9DDFB67E2F199ULL,
   0x0E09B88E1914F7AFULL, 0x10E8B35AF3EEAB37ULL, 0x9EEDECA8E272B933ULL, 0xD4C718BC4AE8AE5FULL,
   0x81536D601170FC20ULL, 0x91B534F885818A06ULL, 0xEC8177F83F900978ULL, 0x190E714FADA5156EULL,
   0xB592BF39B0364963ULL, 0x89C350C893AE7DC1ULL, 0xAC042E70F8B383F2ULL, 0xB49B52E587A1EE60ULL,
   0xFB152FE3FF26DA89ULL, 0x3E666E6F69AE2C15ULL, 0x3B544EBE544C19F9ULL, 0xE805A1E290CF2456ULL,
   0x24B33C9D7ED25117ULL, 0xE74733427B72F0C1ULL, 0x0A804D18B7097475ULL, 0x57E3306D881EDB4FULL,
   0x4AE7D6A36EB5DBCBULL, 0x2D8D5432157064C8ULL, 0xD1E649DE1E7F268BULL, 0x8A328A1CEDFE552CULL,
   0x07A3AEC79624C7DAULL, 0x84547DDC3E203C94ULL, 0x990A98FD5071D263ULL, 0x1A4FF12616EEFC89ULL,
   0xF6F7FD1431714200ULL, 0x30C05B1BA332F41CULL, 0x8D2636B81555A786ULL, 0x46C9FEB55D120902ULL,
   0xCCEC0A73B49C9921ULL, 0x4E9D2827355FC492ULL, 0x19EBB029435DCB0FULL, 0x4659D2B743848A2CULL,
   0x963EF2C96B33BE31ULL, 0x74F85198B05A2E7DULL, 0x5A0F544DD2B1FB18ULL, 0x03727073C2E134B1ULL,
   0xC7F6AA2DE59AEA61ULL, 0x352787BAA0D7C22FULL, 0x9853EAB63B5E0B35ULL, 0xABBDCDD7ED5C0860ULL,
   0xCF05DAF5AC8D77B0ULL, 0x49CAD48CEBF4A71EULL, 0x7A4C10EC2158C4A6ULL, 0xD9E92AA246BF719EULL,
   0x13AE978D09FE5557ULL, 0x730499AF921549FFULL, 0x4E4B705B92903BA4ULL, 0xFF577222C14F0A3AULL,
   0x55B6344CF97AAFAEULL, 0xB862225B055B6960ULL, 0xCAC09AFBDDD2CDB4ULL, 0xDAF8E9829FE96B5FULL,
   0xB5FDFC5D3132C498ULL, 0x310CB380DB6F7503ULL, 0xE87FBB46217A360EULL, 0x2102AE466EBB1148ULL,
   0xF8549E1A3AA5E00DULL, 0x07A69AFDCC42261AULL, 0xC4C118BFE78FEAAEULL, 0xF9F4892ED96BD438ULL,
   0x1AF3DBE25D8F45DAULL, 0xF5B4B0B0D2DEEEB4ULL, 0x962ACEEFA82E1C84ULL, 0x046E3ECAAF453CE9ULL,
   0xF05D129681949A4CULL, 0x964781CE734B3C84ULL, 0x9C2ED44081CE5FBDULL, 0x522E23F3925E319EULL,
   0x177E00F9FC32F791ULL, 0x2BC60A63A6F3B3F2ULL, 0x222BBFAE61725606ULL, 0x486289DDCC3D6780ULL,
   0x7DC7785B8EFDFC80ULL, 0x8AF38731C02BA980ULL, 0x1FAB64EA29A2DDF7ULL, 0xE4D9429322CD065AULL,
   0x9DA058C67844F20CULL, 0x24C0E332B70019B0ULL, 0x233003B5A6CFE6ADULL, 0xD586BD01C5C217F6ULL,
   0x5E5637885F29BC2BULL, 0x7EBA726D8C94094BULL, 0x0A56A5F0BFE39272ULL, 0xD79476A84EE20D06ULL,
   0x9E4C1269BAA4BF37ULL, 0x17EFEE45B0DEE640ULL, 0x1D95B0A5FCF90BC6ULL, 0x93CBE0B699C2585DULL,
   0x65FA4F227A2B6D79ULL, 0xD5F9E858292504D5ULL, 0xC2B5A03F71471A6FULL, 0x59300222B4561E00ULL,
   0xCE2F8642CA0712DCULL, 0x7CA9723FBB2E8988ULL, 0x2785338347F2BA08ULL, 0xC61BB3A141E50E8CULL,
   0x150F361DAB9DEC26ULL, 0x9F6A419D382595F4ULL, 0x64A53DC924FE7AC9ULL, 0x142DE49FFF7A7C3DULL,
   0x0C335248857FA9E7ULL, 0x0A9C32D5EAE45305ULL, 0xE6C42178C4BBB92EULL, 0x71F1CE2490D20B07ULL,
   0xF1BCC3D275AFE51AULL, 0xE728E8C83C334074ULL, 0x96FBF83A12884624ULL, 0x81A1549FD6573DA5ULL,
   0x5FA7867CAF35E149ULL, 0x56986E2EF3ED091BULL, 0x917F1DD5F8886C61ULL, 0xD20D8C88C8FFE65FULL,
   0x31D71DCE64B2C310ULL, 0xF165B587DF898190ULL, 0xA57E6339DD2CF3A0ULL, 0x1EF6E6DBB1961EC9ULL,
   0x70CC73D90BC26E24ULL, 0xE21A6B35DF0C3AD7ULL, 0x003A93D8B2806962ULL, 0x1C99DED33CB890A1ULL,
   0xCF3145DE0ADD4289ULL, 0xD0E4427A5514FB72ULL, 0x77C621CC9FB3A483ULL, 0x67A34DAC4356550BULL,
   0xF8D626AAAF278509ULL
};

const uint64_t polyglotCastle[4] = {
    0x31D71DCE64B2C310ULL,
    0xF165B587DF898190ULL,
    0xA57E6339DD2CF3A0ULL,
    0x1EF6E6DBB1961EC9ULL
};

const uint64_t polyglotEnpassant[8] = {
    0x70CC73D90BC26E24ULL,
    0xE21A6B35DF0C3AD7ULL,
    0x003A93D8B2806962ULL,
    0x1C99DED33CB890A1ULL,
    0xCF3145DE0ADD4289ULL,
    0xD0E4427A5514FB72ULL,
    0x77C621CC9FB3A483ULL,
    0x67A34DAC4356550BULL 
};

const uint64_t polyglotWhite = 0xF8D626AAAF278509ULL;

#endif
