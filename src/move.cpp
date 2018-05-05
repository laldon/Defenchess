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

#define __USE_MINGW_ANSI_STDIO 0

#include "move.h"
#include "target.h"
#include "movegen.h"
#include <cstring>

void insert_piece(Position *p, Square at, Piece piece) {
    Color color = piece_color(piece);
    p->pieces[at] = piece;
    Bitboard bb = bfi[at];
    p->bbs[piece] |= bb;
    p->bbs[color] |= bb;
    p->board |= bb;
}

void remove_piece(Position *p, Square at, Piece piece) {
    Color color = piece_color(piece);
    p->pieces[at] = empty;
    Bitboard bb = bfi[at];
    p->bbs[piece] ^= bb;
    p->bbs[color] ^= bb;
    p->board ^= bb;
}

void move_piece(Position *p, Square from, Square to, Piece piece, Color curr_c) {
    p->pieces[from] = empty;
    p->pieces[to] = piece;
    Bitboard from_to = bfi[from] ^ bfi[to];
    p->bbs[piece] ^= from_to;
    p->bbs[curr_c] ^= from_to;
    p->board ^= from_to;

    uint64_t h = polyglotCombined[piece][from] ^ polyglotCombined[piece][to];
    Info *info = p->info;
    info->hash ^= h;
    if (is_pawn(piece)) {
        info->pawn_hash ^= h;
    }
    info->score += pst[piece][to] - pst[piece][from];
}

void capture(Position *p, Square to, Piece captured, Color opponent) {
    p->bbs[captured] ^= bfi[to];
    p->bbs[opponent] ^= bfi[to];
    p->board ^= bfi[to];

    uint64_t h = polyglotCombined[captured][to];
    Info *info = p->info;
    info->hash ^= h;
    if (is_pawn(captured)) {
        info->pawn_hash ^= h;
    } else {
        info->non_pawn_material[opponent] -= piece_values[captured];
    }
    info->material_index -= material_balance[captured];
    info->score -= pst[captured][to];
}

void capture_enpassant(Position *p, Square to, Square enpassant_to, Piece captured, Color opponent) {
    p->bbs[captured] ^= bfi[enpassant_to];
    p->bbs[opponent] ^= bfi[enpassant_to];
    p->pieces[enpassant_to] = empty;
    p->board ^= bfi[enpassant_to];

    uint64_t h = polyglotCombined[captured][to];
    Info *info = p->info;
    info->hash ^= h;
    info->pawn_hash ^= h;
    info->material_index -= material_balance[captured];
    info->score -= pst[captured][to];
}

void promote(Position *p, Square to, Piece pawn, Piece promotion_type, Color color) {
    p->pieces[to] = promotion_type;
    p->bbs[pawn] ^= bfi[to];
    p->bbs[promotion_type] ^= bfi[to];

    uint64_t h = polyglotCombined[pawn][to];
    Info *info = p->info;
    info->pawn_hash ^= h;
    info->hash ^= h ^ polyglotCombined[promotion_type][to];
    info->material_index += material_balance[promotion_type] - material_balance[pawn];
    info->non_pawn_material[color] += piece_values[promotion_type];
    info->score += pst[promotion_type][to] - pst[pawn][to];
}


void make_move(Position *p, Move move) {
    SearchThread *my_thread = p->my_thread;
    ++my_thread->search_ply;

    Info *info = p->info;
    std::memcpy(info + 1, info, info_size);
    Info *new_info = info + 1;
    p->info = new_info;
    new_info->previous = info;

    ++new_info->last_irreversible;
    Square from = move_from(move);
    Square to = move_to(move);
    Color curr_c = p->color;
    Color opponent = opponent_color(curr_c);
    Piece piece = p->pieces[from];
    Move m_type = move_type(move);
    Piece captured = m_type == ENPASSANT ? pawn(opponent) : p->pieces[to];

    if (!(piece_color(piece) == curr_c)) {
        std::cout << "piece: " << int(piece) << ", color: " << int(curr_c) << std::endl;
    }
    assert(piece_color(piece) == curr_c);
    assert(!is_king(captured));

    new_info->enpassant = 0;
    new_info->hash = info->hash ^ polyglotWhite;
    p->color = opponent;

    if (info->enpassant) {
        // Clear the enpassant hash from the previous position
        new_info->hash ^= polyglotEnpassant[col(info->enpassant)];
    }

    if (captured) {
        new_info->last_irreversible = 0;
        if (m_type == ENPASSANT) {
            Square enpassant_to = ENPASSANT_INDEX[to];
            assert(piece == pawn(curr_c));
            assert(to == info->enpassant);
            assert(info->enpassant != 0);
            assert(rank(to, curr_c) == RANK_6);
            assert(p->pieces[to] == empty);
            assert(p->pieces[enpassant_to] == pawn(opponent));

            capture_enpassant(p, to, enpassant_to, captured, opponent);
        } else {
            capture(p, to, captured, opponent);
        }
    }

    move_piece(p, from, to, piece, curr_c);
    if (is_king(piece)) {
        p->king_index[curr_c] = to;
        if (m_type == CASTLING) {
            new_info->last_irreversible = 0;
            move_piece(p, ROOK_MOVES_CASTLE_FROM[to], ROOK_MOVES_CASTLE_TO[to], ROOK_MOVES_CASTLE_PIECE[to], curr_c);
        }
    } else if (is_pawn(piece)) {
        new_info->last_irreversible = 0;
        if ((from ^ to) == 16) {
            if (p->bbs[pawn(opponent)] & ADJACENT_MASK[to]) {
                new_info->enpassant = ENPASSANT_INDEX[from];
                new_info->hash ^= polyglotEnpassant[col(new_info->enpassant)];
            }
        } else if (m_type == PROMOTION) {
            assert(rank(to, curr_c) == RANK_8);
            promote(p, to, piece, promotion_piece(move, curr_c), curr_c);
        }
    }

    if (new_info->castling && CASTLING_RIGHTS[from] & CASTLING_RIGHTS[to]) {
        int castling_rights = CASTLING_RIGHTS[from] & CASTLING_RIGHTS[to];
        new_info->castling &= castling_rights;
        new_info->hash ^= castlingHash[castling_rights];
    }

    new_info->captured = captured;
    new_info->pinned[white] = pinned_piece_squares(p, white);
    new_info->pinned[black] = pinned_piece_squares(p, black);

    // assert(is_position_valid(p));
}

void make_null_move(Position *p) {
    SearchThread *my_thread = p->my_thread;
    ++my_thread->search_ply;

    Info *info = p->info;
    std::memcpy(info + 1, info, info_size);
    Info *new_info = info + 1;
    p->info = new_info;
    new_info->previous = info;

    ++new_info->last_irreversible;

    if (info->enpassant) {
        new_info->hash ^= polyglotEnpassant[col(info->enpassant)];
        new_info->enpassant = 0;
    }
    new_info->hash ^= polyglotWhite;
    p->color ^= 1;
}

void undo_move(Position *p, Move move) {
    SearchThread *my_thread = p->my_thread;
    --my_thread->search_ply;
    p->color ^= 1;

    Info *info = p->info;
    Color color = p->color;
    Square from = move_from(move);
    Square to = move_to(move);
    Piece piece = p->pieces[to];

    assert(!is_king(info->captured));
    assert(p->pieces[from] == empty);

    if (is_king(piece)) {
        p->king_index[color] = from;
    }
    if (move_type(move) == NORMAL) {
        move_piece(p, to, from, piece, color);
        if (info->captured) {
            insert_piece(p, to, info->captured);
        }
    } else if (move_type(move) == PROMOTION) {
        remove_piece(p, to, piece);
        insert_piece(p, from, pawn(color));
        if (info->captured) {
            insert_piece(p, to, info->captured);
        }
    } else if (move_type(move) == CASTLING) {
        Square rook_from = relative_square(col(to) == FILE_G ? H1 : A1, color);
        Square rook_to = relative_square(col(to) == FILE_G ? F1 : D1, color);
        remove_piece(p, to, piece);
        remove_piece(p, rook_to, rook(color));
        insert_piece(p, from, piece);
        insert_piece(p, rook_from, rook(color));
    } else { // Enpassant
        move_piece(p, to, from, piece, color);
        assert(is_pawn(piece));
        assert(to == info->previous->enpassant);
        assert(rank(to, color) == RANK_6);
        assert(info->captured == pawn(opponent_color(color)));
        insert_piece(p, pawn_backward(to, color), info->captured);
    }
    p->info = info->previous;
}

void undo_null_move(Position *p) {
    p->info = p->info->previous;
    p->color ^= 1;
}

bool is_pseudolegal(Position *p, Move move) {
    Square from = move_from(move);
    Piece piece = p->pieces[from];
    Piece p_type = piece_type(piece);
    Move m_type = move_type(move);

    if (m_type != NORMAL) {
        MoveGen movegen = blank_movegen;
        movegen.position = p;
        generate_moves<ALL>(&movegen, p);
        for (uint8_t i = movegen.head; i < movegen.tail; ++i) {
            Move gen_move = movegen.moves[i].move;
            if (move == gen_move) {
                return true;
            }
        }
        return false;
    }

    if (promotion_type(move)) {
        return false;
    }

    if (piece == empty) {
        return false;
    }
    if (piece_color(piece) != p->color) {
        return false;
    }

    Square to = move_to(move);
    if (p->bbs[p->color] & bfi[to]) {
        return false;
    }

    Bitboard b = 0;
    if (p_type == white_king) {
        b = generate_king_targets(from);
    } else if (p_type == white_pawn) {
        if (rank(to, p->color) == RANK_8) {
            return false;
        }
        if (col(from) != col(to) && !(p->board & bfi[to])) {  // Enpassant
            return false;
        }
        b = generate_pawn_targets<ALL>(p, from);
    } else if (p_type == white_knight) {
        b = generate_knight_targets(from);
    } else if (p_type == white_bishop) {
        b = generate_bishop_targets(p->board, from);
    } else if (p_type == white_rook) {
        b = generate_rook_targets(p->board, from);
    } else if (p_type == white_queen) {
        b = generate_queen_targets(p->board, from);
    }
    return b & bfi[to];
}

bool gives_check(Position *p, Move m) {
    Square their_king_index = p->king_index[opponent_color(p->color)];
    Bitboard their_king = bfi[their_king_index];

    Square from = move_from(m);
    Square to = move_to(m);
    Piece curr_piece = p->pieces[from];

    if (piece_type(curr_piece) == white_pawn) {
        //? Targets King at location:
        if (PAWN_CAPTURE_MASK[to][p->color] & their_king)
            return true;

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

        //? Promotion check ?
        if (move_type(m) == PROMOTION) {
            if (is_queen_promotion(m)) {
                if (generate_queen_targets(p->board ^ bfi[from], to) & their_king)
                    return true;
            } else if (is_rook_promotion(m)) {
                if (generate_rook_targets(p->board ^ bfi[from], to) & their_king)
                    return true;
            } else if (is_bishop_promotion(m)) {
                if (generate_bishop_targets(p->board ^ bfi[from], to) & their_king)
                    return true;
            } else if (is_knight_promotion(m)) {
                if (generate_knight_targets(to) & their_king)
                    return true;
            }
        }

        if (move_type(m) == ENPASSANT) {
            make_move(p, m);
            undo_move(p, m);
            if (is_checked(p)) {
                return true;
            }
        } 

    } else if (piece_type(curr_piece) == white_king) {

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

        //? Is a rook check ?
        if (move_type(m) == CASTLING) {
            make_move(p, m);
            undo_move(p, m);
            if (is_checked(p)) {
                return true;
            }
        }

    } else if (piece_type(curr_piece) == white_queen) {
        //? Targets King at location:
        if (generate_queen_targets(p->board, to) & their_king)
            return true;

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

    } else if (piece_type(curr_piece) == white_rook) {
        //? Targets King at location:
        if (generate_rook_targets(p->board, to) & their_king)
            return true;

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

    } else if (piece_type(curr_piece) == white_bishop) {
        //? Targets King at location:
        if (generate_bishop_targets(p->board, to) & their_king)
            return true;

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

    } else if (piece_type(curr_piece) == white_knight) {
        //? Targets King at location:
        if (KNIGHT_MASKS[to] & their_king)
            return true;

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

    }

    return false;
}
