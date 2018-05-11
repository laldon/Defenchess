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

#include "position.h"
#include <iostream>

void calculate_score(Position *p) {
    p->score = Score{0, 0};
    Bitboard board = p->board;

    while (board) {
        Square outpost = pop(&board);
        Piece piece = p->pieces[outpost];
        p->score += pst[piece][outpost];
    }
}

void calculate_hash(Position *p) {
    Bitboard board = p->board;

    while(board) {
        Square square = pop(&board);
        Piece piece = p->pieces[square];
        uint64_t hash = polyglotArray[polyglotPieces[piece] + square];
        p->hash ^= hash;
        if (is_pawn(piece)) {
            p->pawn_hash ^= hash;
        }
    }

    p->hash ^= castlingHash[p->castling];
    if (p->enpassant) {
        p->hash ^= polyglotEnpassant[col(p->enpassant)];
    }
    if (p->color == white) {
        p->hash ^= polyglotWhite;
    }
}

void calculate_material(Position *p) {
    p->material_index = 0;
    int wp = count(p->bbs[pawn(white)]);
    int wn = count(p->bbs[knight(white)]);
    int wb = count(p->bbs[bishop(white)]);
    int wr = count(p->bbs[rook(white)]);
    int wq = count(p->bbs[queen(white)]);

    int bp = count(p->bbs[pawn(black)]);
    int bn = count(p->bbs[knight(black)]);
    int bb = count(p->bbs[bishop(black)]);
    int br = count(p->bbs[rook(black)]);
    int bq = count(p->bbs[queen(black)]);

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

    p->material_index = index;

    p->non_pawn_material[white] = wn * KNIGHT_MID +
                                  wb * BISHOP_MID +
                                  wr * ROOK_MID +
                                  wq * QUEEN_MID;

    p->non_pawn_material[black] = bn * KNIGHT_MID +
                                  bb * BISHOP_MID +
                                  br * ROOK_MID +
                                  bq * QUEEN_MID;
}

Position* add_pieces(Position* p){
    for (int i = 0; i < 64; ++i) {
        if (on(p->board, i)) {
            for (int j = 2; j < 14; ++j) {
                if (on(p->bbs[j], i)) {
                    p->pieces[i] = j;
                    break;
                } else {
                    p->pieces[i] = no_piece;
                }
            }
        } else {
            p->pieces[i] = no_piece;
        }
    }
    return p;
}

Position* import_fen(const char* fen){
    char *buffer = (char *)calloc(strlen(fen) + 1, 1);
    strcat(buffer, fen);

    char *matches[6] = {};
    char *token = strtok(buffer, " ");
    for (int i = 0; i < 6 && token; i++) {
        matches[i] = token;
        token = strtok(NULL, " ");
    }
    
    Color color;
    if (matches[1][0] == 'w') {
        color = white;
    } else if (matches[1][0] == 'b') {
        color = black;
    } else {
        assert(false);
        return 0;
    }

    int halfmove_clock;
    if (matches[5]) {
        halfmove_clock = atoi(matches[5]);
    } else {
        halfmove_clock = 1;
    }
    root_ply = 2 * (halfmove_clock - 1) + color;

    SearchThread *main_thread = &search_threads[0];
    Position *p = &(main_thread->positions[root_ply]);
    main_thread->search_ply = root_ply;
    p->color = color;

    p->bbs[white_occupy] = 0;
    p->bbs[black_occupy] = 0;

    p->bbs[white_knight] = 0;
    p->bbs[white_rook] = 0;
    p->bbs[white_bishop] = 0;
    p->bbs[white_pawn] = 0;
    p->bbs[white_king] = 0;
    p->bbs[white_queen] = 0;

    p->bbs[black_knight] = 0;
    p->bbs[black_rook] = 0;
    p->bbs[black_bishop] = 0;
    p->bbs[black_pawn] = 0;
    p->bbs[black_king] = 0;
    p->bbs[black_queen] = 0;
    
    p->board = 0;

    for (int i = 0 ; i < 64 ; i++){
        p->pieces[i] = no_piece;
    }

    int sq = A8;
    for (char *ch = matches[0]; *ch; ch++) {
        Piece piece = (Piece) 32;
        switch(*ch) {
        case 'P':
            piece = white_pawn;
            break;
        case 'p':
            piece = black_pawn;
            break;
        case 'N':
            piece = white_knight;
            break;
        case 'n':
            piece = black_knight;
            break;
        case 'B':
            piece = white_bishop;
            break;
        case 'b':
            piece = black_bishop;
            break;
        case 'R':
            piece = white_rook;
            break;
        case 'r':
            piece = black_rook;
            break;
        case 'Q':
            piece = white_queen;
            break;
        case 'q':
            piece = black_queen;
            break;
        case 'K':
            piece = white_king;
            p->king_index[white] = (Square) sq;
            break;
        case 'k':
            piece = black_king;
            p->king_index[black] = (Square) sq;
            break;
        case '/':
            sq -= 16;
            break;
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
            sq += *ch - '0';
        }
        if (piece != 32) {
            p->pieces[sq] = piece;
            p->bbs[piece] |= bfi(sq);
            p->bbs[piece_color(piece)] |= bfi(sq++);
        }
        
    }

    p->castling = 0;
    for (char *ch = matches[2]; *ch; ch++) {
        switch(*ch) {
        case 'K':
            p->castling |= 1;
            break;
        case 'Q':
            p->castling |= 2;
            break;
        case 'k':
            p->castling |= 4;
            break;
        case 'q':
            p->castling |= 8;
            break;
        case '-':
            break;
        }
    }

    if (matches[3][0] != '-') {
        p->enpassant = (Square)(((matches[3][1] - '1') << 3) + matches[3][0] - 'a');
    } else {
        p->enpassant = 0;
    }

    if (matches[4]) {
        p->last_irreversible = atoi(matches[4]);
    } else {
        p->last_irreversible = 0;
    }
    p->board = p->bbs[white] | p->bbs[black];
    p->hash = p->pawn_hash = 0;
    p->pinned[white] = pinned_piece_squares(p, white);
    p->pinned[black] = pinned_piece_squares(p, black);
    calculate_score(p);
    calculate_hash(p);
    calculate_material(p);
    free(buffer);
    p->my_thread = main_thread;
    return p;
}

Position* start_pos(){
    SearchThread *main_thread = &search_threads[0];
    Position *p = &(main_thread->positions[0]);

    Bitboard white_occupied_bb = 0x000000000000FFFF;
    Bitboard black_occupied_bb = 0xFFFF000000000000;

    Bitboard white_rooks_bb = 0x0000000000000081;
    Bitboard white_knights_bb = 0x0000000000000042;
    Bitboard white_bishops_bb = 0x0000000000000024;
    Bitboard white_king_bb = 0x0000000000000010;
    Bitboard white_queen_bb = 0x0000000000000008;
    Bitboard white_pawns_bb = 0x000000000000FF00;

    Bitboard black_rooks_bb = 0x8100000000000000;
    Bitboard black_knights_bb = 0x4200000000000000;
    Bitboard black_bishops_bb = 0x2400000000000000;
    Bitboard black_king_bb = 0x1000000000000000;
    Bitboard black_queen_bb = 0x0800000000000000;
    Bitboard black_pawns_bb = 0x00FF000000000000;

    p->bbs[white_occupy] = white_occupied_bb;
    p->bbs[black_occupy] = black_occupied_bb;

    p->bbs[white_knight] = white_knights_bb;
    p->bbs[white_rook] = white_rooks_bb;
    p->bbs[white_bishop] = white_bishops_bb;
    p->bbs[white_pawn] = white_pawns_bb;
    p->bbs[white_king] = white_king_bb;
    p->bbs[white_queen] = white_queen_bb;

    p->bbs[black_knight] = black_knights_bb;
    p->bbs[black_rook] = black_rooks_bb;
    p->bbs[black_bishop] = black_bishops_bb;
    p->bbs[black_pawn] = black_pawns_bb;
    p->bbs[black_king] = black_king_bb;
    p->bbs[black_queen] = black_queen_bb;
    
    p->board = black_occupied_bb | white_occupied_bb;
    p->king_index[white] = E1;
    p->king_index[black] = E8;
    add_pieces(p);
    p->color = white;
    p->castling = 15;
    p->enpassant = 0;
    p->hash = p->pawn_hash = 0;
    main_thread->search_ply = root_ply = 0;
    p->pinned[white] = pinned_piece_squares(p, white);
    p->pinned[black] = pinned_piece_squares(p, black);
    calculate_score(p);
    calculate_hash(p);
    calculate_material(p);
    p->my_thread = main_thread;
    p->last_irreversible = 0;
    return p;
}
