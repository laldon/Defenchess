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

void move_piece(Position *p, Square from, Square to, Piece piece, Color curr_c) {
    p->pieces[from] = empty;
    p->pieces[to] = piece;
    Bitboard from_to = bfi[from] ^ bfi[to];
    p->bbs[piece] ^= from_to;
    p->bbs[curr_c] ^= from_to;
    p->board ^= from_to;

    uint64_t h = polyglotCombined[piece][from] ^ polyglotCombined[piece][to];
    p->hash ^= h;
    if (is_pawn(piece)) {
        p->pawn_hash ^= h;
    }
    p->score += pst[piece][to] - pst[piece][from];
}

void capture(Position *p, Square to, Piece captured, Color opponent) {
    p->bbs[captured] ^= bfi[to];
    p->bbs[opponent] ^= bfi[to];
    p->board ^= bfi[to];

    uint64_t h = polyglotCombined[captured][to];
    p->hash ^= h;
    if (is_pawn(captured)) {
        p->pawn_hash ^= h;
    } else {
        p->non_pawn_material[opponent] -= piece_values[captured];
    }
    p->material_index -= material_balance[captured];
    p->score -= pst[captured][to];
}

void capture_enpassant(Position *p, Square to, Square enpassant_to, Piece captured, Color opponent) {
    p->bbs[captured] ^= bfi[enpassant_to];
    p->bbs[opponent] ^= bfi[enpassant_to];
    p->pieces[enpassant_to] = empty;
    p->board ^= bfi[enpassant_to];

    uint64_t h = polyglotCombined[captured][to];
    p->hash ^= h;
    p->pawn_hash ^= h;
    p->material_index -= material_balance[captured];
    p->score -= pst[captured][to];
}

void promote(Position *p, Square to, Piece pawn, Piece promotion_type, Color color) {
    p->pieces[to] = promotion_type;
    p->bbs[pawn] ^= bfi[to];
    p->bbs[promotion_type] ^= bfi[to];

    uint64_t h = polyglotCombined[pawn][to];
    p->pawn_hash ^= h;
    p->hash ^= h ^ polyglotCombined[promotion_type][to];
    p->material_index += material_balance[promotion_type] - material_balance[pawn];
    p->non_pawn_material[color] += piece_values[promotion_type];
    p->score += pst[promotion_type][to] - pst[pawn][to];
}


Position *make_move(Position *p, Move move) {
    SearchThread *my_thread = p->my_thread;
    ++my_thread->search_ply;

    std::memcpy(my_thread->positions + my_thread->search_ply, my_thread->positions + my_thread->search_ply - 1, position_size);

    Position *new_p = &(my_thread->positions[my_thread->search_ply]);
    ++new_p->last_irreversible;
    Square from = move_from(move);
    Square to = move_to(move);
    Color curr_c = p->color;
    Color opponent = opponent_color(curr_c);
    Piece piece = p->pieces[from];
    Move m_type = move_type(move);
    Piece captured = m_type == ENPASSANT ? pawn(opponent) : p->pieces[to];

    assert(piece_color(piece) == curr_c);
    assert(!is_king(captured));

    new_p->enpassant = 0;
    new_p->hash = p->hash ^ polyglotWhite;
    new_p->color = opponent;

    if (p->enpassant) {
        // Clear the enpassant hash from the previous position
        new_p->hash ^= polyglotEnpassant[col(p->enpassant)];
    }

    if (captured) {
        new_p->last_irreversible = 0;
        if (m_type == ENPASSANT) {
            Square enpassant_to = ENPASSANT_INDEX[to];
            assert(piece == pawn(curr_c));
            assert(to == p->enpassant);
            assert(p->enpassant != 0);
            assert(rank(to, curr_c) == RANK_6);
            assert(p->pieces[to] == empty);
            assert(p->pieces[enpassant_to] == pawn(opponent));

            capture_enpassant(new_p, to, enpassant_to, captured, opponent);
        } else {
            capture(new_p, to, captured, opponent);
        }
    }

    move_piece(new_p, from, to, piece, curr_c);
    if (is_king(piece)) {
        new_p->king_index[curr_c] = to;
        if (m_type == CASTLING) {
            new_p->last_irreversible = 0;
            move_piece(new_p, ROOK_MOVES_CASTLE_FROM[to], ROOK_MOVES_CASTLE_TO[to], ROOK_MOVES_CASTLE_PIECE[to], curr_c);
        }
    } else if (is_pawn(piece)) {
        new_p->last_irreversible = 0;
        if ((from ^ to) == 16) {
            if (new_p->bbs[pawn(opponent)] & ADJACENT_MASK[to]) {
                new_p->enpassant = ENPASSANT_INDEX[from];
                new_p->hash ^= polyglotEnpassant[col(new_p->enpassant)];
            }
        } else if (m_type == PROMOTION) {
            assert(rank(to, curr_c) == RANK_8);
            promote(new_p, to, piece, promotion_piece(move, curr_c), curr_c);
        }
    }

    if (new_p->castling && CASTLING_RIGHTS[from] & CASTLING_RIGHTS[to]) {
        int castling_rights = CASTLING_RIGHTS[from] & CASTLING_RIGHTS[to];
        new_p->castling &= castling_rights;
        new_p->hash ^= castlingHash[castling_rights];
    }

    new_p->pinned[white] = pinned_piece_squares(new_p, white);
    new_p->pinned[black] = pinned_piece_squares(new_p, black);

    assert(is_position_valid(new_p));
    return new_p;
}

Position *make_null_move(Position *p) {
    SearchThread *my_thread = p->my_thread;
    ++my_thread->search_ply;
    std::memcpy(my_thread->positions + my_thread->search_ply, my_thread->positions + my_thread->search_ply - 1, sizeof(Position));
    Position *new_p = &(my_thread->positions[my_thread->search_ply]);
    ++new_p->last_irreversible;

    if (p->enpassant) {
        new_p->hash ^= polyglotEnpassant[col(p->enpassant)];
        new_p->enpassant = 0;
    }
    new_p->hash ^= polyglotWhite;
    new_p->color ^= 1;
    return new_p;
}

Position *undo_move(Position *p) {
    SearchThread *my_thread = p->my_thread;
    --my_thread->search_ply;
    return &(my_thread->positions[my_thread->search_ply]);
}

bool is_pseudolegal(Position *p, Move move) {
    Square from = move_from(move);
    Piece piece = p->pieces[from];
    Piece p_type = piece_type(piece);
    Move m_type = move_type(move);

    if (m_type != NORMAL) {
        MoveGen movegen = blank_movegen;
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
    return b & bfi[to] & ~p->bbs[p->color];
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
            Position *position = make_move(p, m);
            undo_move(p);
            if (is_checked(position)) {
                return true;
            }
        } 

    } else if (piece_type(curr_piece) == white_king) {

        //? Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, opponent_color(p->color)), from))
            return true;

        //? Is a rook check ?
        if (move_type(m) == CASTLING) {
            Position *position = make_move(p, m);
            undo_move(p);
            if (is_checked(position)) {
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
