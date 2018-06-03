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

#include "eval.h"
#include "bitboard.h"
#include "target.h"
#include "tt.h"
#include "move.h"
#include "endgame.h"
#include <iostream>

Score connected[2][2][3][8];

void init_eval() {
    for (int opposed = 0; opposed <= 1; ++opposed)
        for (int adjacent = 0; adjacent <= 1; ++adjacent)
            for (int supported = 0; supported <= 2; ++supported)
                for (int r = RANK_2; r < RANK_8; ++r)
    {
        int v = 15 * supported + ((connection_bonus[r] + (adjacent ? (connection_bonus[r + 1] - connection_bonus[r]) / 2 : 0)) >> opposed);
        connected[opposed][adjacent][supported][r] = Score{v, v * (r - 2) / 4};
    }
}

Score evaluate_pawn_structure(Evaluation *eval, Position *p, Color color) {
    Score pawn_structure = {0, 0};

    Color opp_c = opponent_color(color);
    Bitboard my_pawns = p->bbs[pawn(color)];
    Bitboard opponent_pawns = p->bbs[pawn(opp_c)];
    Bitboard pawns = my_pawns;
    Bitboard doubled, supported, backward, threatening, adjacent;
    bool opposed, passing, isolated;

    while (pawns) {
        Square outpost = pop(&pawns);
        int pawn_column = col(outpost);
        int pawn_rank = rank(outpost, color);

        supported = p->bbs[pawn(color)] & PAWN_CAPTURE_MASK[outpost][opp_c];
        doubled = FRONT_MASK[outpost][color] & my_pawns;
        threatening = PAWN_CAPTURE_MASK[outpost][color] & opponent_pawns;
        adjacent = ADJACENT_MASK[outpost] & my_pawns;

        // Passing or candidate passing
        passing = ((PASSED_PAWN_MASK[outpost][color] & opponent_pawns) == 0) ||
                  ((PASSED_PAWN_MASK[pawn_forward(outpost, color)][color] & opponent_pawns) == 0);
        isolated = (MASK_ISOLATED[pawn_column] & my_pawns) == 0;
        opposed = FRONT_MASK[outpost][color] & opponent_pawns;

        backward = false;
        if (!passing && !supported && !isolated && !threatening && pawn_rank < RANK_4) {
            if ((PASSED_PAWN_MASK[outpost][opp_c] & MASK_ISOLATED[pawn_column] & my_pawns) == 0) {
                if (bfi[pawn_forward(outpost, color)] & eval->targets[pawn(opp_c)]) {
                    backward = true;
                }
            }
        }

        if (supported | adjacent) {
            pawn_structure += connected[opposed][(bool) adjacent][count(supported)][rank(outpost, color)];
        } else if (isolated) {
            pawn_structure -= isolated_pawn_penalty[opposed];
        } else if (backward) {
            pawn_structure -= backward_pawn_penalty[opposed];
        }
        if (doubled) {
            pawn_structure -= double_pawn_penalty;
        }
        if (passing) {
            eval->pawn_passers[color] |= bfi[outpost];
        }

        // King threats
        eval->semi_open_files[color] &= ~(1 << col(outpost));
        Bitboard king_threats = PAWN_CAPTURE_MASK[outpost][color] & eval->king_zone[opp_c];
        if (king_threats) {
            ++eval->num_king_attackers[opp_c];
            eval->king_zone_score[opp_c] += ATTACK_VALUES[PAWN];
        }
        ++eval->num_pieces[pawn(color)];
        ++eval->num_pieces[color];
    }
    return pawn_structure;
}

void evaluate_pawn_init(Evaluation *eval, Position *p, Color color) {
    // Do stuff here that's needed for other evaluations because
    // evaluating the pawn structure is skipped in the case of a pawntte hit
    Color opp_c = opponent_color(color);
    Bitboard my_pawns = p->bbs[pawn(color)];
    while (my_pawns) {
        Square outpost = pop(&my_pawns);
        // King threats
        Bitboard king_threats = PAWN_CAPTURE_MASK[outpost][color] & eval->king_zone[opp_c];
        if (king_threats) {
            ++eval->num_king_attackers[opp_c];
            eval->king_zone_score[opp_c] += ATTACK_VALUES[PAWN];
        }
        ++eval->num_pieces[pawn(color)];
        ++eval->num_pieces[color];
    }
}

void evaluate_pawns(Evaluation *eval, Position *p) {
    PawnTTEntry *pawntte = get_pawntte(p->pawn_hash);
    if (pawntte) {
        evaluate_pawn_init(eval, p, white);
        evaluate_pawn_init(eval, p, black);
        eval->score_pawn = pawntte->score;
        eval->pawn_passers[white] = pawntte->pawn_passers[white];
        eval->pawn_passers[black] = pawntte->pawn_passers[black];
        eval->semi_open_files[white] = pawntte->semi_open_files[white];
        eval->semi_open_files[black] = pawntte->semi_open_files[black];
        return;
    }

    Score whitey = evaluate_pawn_structure(eval, p, white);
    Score blacky = evaluate_pawn_structure(eval, p, black);
    eval->score_pawn = whitey - blacky;

    set_pawntte(p->pawn_hash, eval);
}

Score evaluate_bishop(Evaluation *eval, Position *p, Color color) {
    Score bishop_score = {0, 0};
    Color opp_c = opponent_color(color);
    Bitboard my_bishops = p->bbs[bishop(color)];

    while (my_bishops) {
        Square outpost = pop(&my_bishops);
        eval->bishop_squares[color] = outpost;
        Bitboard bishop_targets = generate_bishop_targets(p->board ^ p->bbs[queen(color)], outpost);

        if (p->pinned[color] & bfi[outpost])
            bishop_targets &= BETWEEN_MASK[p->king_index[color]][outpost];

        // Protected bishop
        if (bfi[outpost] & eval->targets[pawn(color)]) {
            bishop_score += protected_piece_bonus;
        }

        // Bishop with same colored pawns
        bishop_score -= bishop_pawn_penalty * count(COLOR_MASKS[TILE_COLOR[outpost]] & p->bbs[pawn(color)]);

        // Hidden behind pawn
        if (rank(outpost, color) < RANK_5 && is_pawn(p->pieces[pawn_forward(outpost, color)])) {
            bishop_score += minor_piece_behind_pawn;
        }

        // Mobility
        int mobility = count(bishop_targets & eval->mobility_area[color]);
        eval->mobility_score[color] += mobility_bonus[BISHOP][mobility];

        // King threats
        Bitboard king_threats = bishop_targets & eval->king_zone[opp_c];
        if (king_threats) {
            ++eval->num_king_attackers[opp_c];
            eval->king_zone_score[opp_c] += ATTACK_VALUES[BISHOP];
            eval->num_king_zone_attacks[opp_c] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & bishop_targets;
        eval->targets[color] |= eval->targets[bishop(color)] |= bishop_targets;
        ++eval->num_pieces[bishop(color)];
        ++eval->num_pieces[color];
    }
    return bishop_score;
}

Score evaluate_knight(Evaluation *eval, Position *p, Color color) {
    Score knight_score = {0, 0};
    Color opp_c = opponent_color(color);
    Bitboard my_knights = p->bbs[knight(color)];

    while (my_knights) {
        Square outpost = pop(&my_knights);
        Bitboard knight_targets = generate_knight_targets(outpost);

        if (p->pinned[color] & bfi[outpost])
            knight_targets &= BETWEEN_MASK[p->king_index[color]][outpost];

        // Protected knight
        if (bfi[outpost] & eval->targets[pawn(color)]) {
            knight_score += protected_piece_bonus;
        }

        // Hidden behind pawn
        if (rank(outpost, color) < RANK_5 && is_pawn(p->pieces[pawn_forward(outpost, color)])) {
            knight_score += minor_piece_behind_pawn;
        }

        // Mobility
        int mobility = count(knight_targets & eval->mobility_area[color]);
        eval->mobility_score[color] += mobility_bonus[KNIGHT][mobility];

        // King threats
        Bitboard king_threats = knight_targets & eval->king_zone[opp_c];
        if (king_threats) {
            ++eval->num_king_attackers[opp_c];
            eval->king_zone_score[opp_c] += ATTACK_VALUES[KNIGHT];
            eval->num_king_zone_attacks[opp_c] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & knight_targets;
        eval->targets[color] |= eval->targets[knight(color)] |= knight_targets;
        ++eval->num_pieces[knight(color)];
        ++eval->num_pieces[color];
    }
    return knight_score;
}

Score evaluate_rook(Evaluation *eval, Position *p, Color color) {
    Score rook_score = {0, 0};
    Color opp_c = opponent_color(color);
    Bitboard my_rooks = p->bbs[rook(color)];

    while (my_rooks) {
        Square outpost = pop(&my_rooks);
        int rook_column = col(outpost);
        Bitboard rook_targets = generate_rook_targets(p->board ^ (p->bbs[queen(color)] | p->bbs[rook(color)]), outpost);

        if (p->pinned[color] & bfi[outpost])
            rook_targets &= BETWEEN_MASK[p->king_index[color]][outpost];

        // Mobility
        int mobility = count(rook_targets & eval->mobility_area[color]);
        eval->mobility_score[color] += mobility_bonus[ROOK][mobility];

        // Rooks threatening enemy pawns
        if (rank(outpost, color) > RANK_4) {
            rook_score += rook_pawn_bonus * count(rook_targets & p->pieces[pawn(opp_c)]);
        }

        // Bonus for being on a semiopen or open file
        if (eval->semi_open_files[color] & (1 << rook_column)) {
            rook_score += rook_file_bonus[!!(eval->semi_open_files[opp_c] & (1 << rook_column))];
        }

        // King blocking the rook
        if (mobility <= 3) {
            if (
                (outpost == relative_square(H1, color) &&
                (p->king_index[color] == relative_square(F1, color) || p->king_index[color] == relative_square(G1, color))) ||
                (outpost == relative_square(A1, color) &&
                (p->king_index[color] == relative_square(B1, color) || p->king_index[color] == relative_square(C1, color)))
            ) {
                rook_score -= blocked_rook_penalty;
            }
        }

        // King threats
        Bitboard king_threats = rook_targets & eval->king_zone[opp_c];
        if (king_threats) {
            ++eval->num_king_attackers[opp_c];
            eval->king_zone_score[opp_c] += ATTACK_VALUES[ROOK];
            eval->num_king_zone_attacks[opp_c] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & rook_targets;
        eval->targets[color] |= eval->targets[rook(color)] |= rook_targets;
        ++eval->num_pieces[rook(color)];
        ++eval->num_pieces[color];
    }
    return rook_score;
}

Score evaluate_queen(Evaluation *eval, Position *p, Color color) {
    Score queen_score = {0, 0};
    Color opp_c = opponent_color(color);
    Bitboard my_queens = p->bbs[queen(color)];

    while (my_queens) {
        Square outpost = pop(&my_queens);
        Bitboard queen_targets = generate_queen_targets(p->board, outpost);

        if (p->pinned[color] & bfi[outpost])
            queen_targets &= BETWEEN_MASK[p->king_index[color]][outpost];

        // Mobility
        int mobility = count(queen_targets & eval->mobility_area[color]);
        eval->mobility_score[color] += mobility_bonus[QUEEN][mobility];

        // King threats
        Bitboard king_threats = queen_targets & eval->king_zone[opp_c];
        if (king_threats) {
            ++eval->num_king_attackers[opp_c];
            eval->king_zone_score[opp_c] += ATTACK_VALUES[QUEEN];
            eval->num_king_zone_attacks[opp_c] += count(king_threats);
        }
        eval->double_targets[color] |= eval->targets[color] & queen_targets;
        eval->targets[color] |= eval->targets[queen(color)] |= queen_targets;
        ++eval->num_pieces[queen(color)];
        ++eval->num_pieces[color];
    }
    return queen_score;
}

int evaluate_pawn_shelter(Position *p, Color color, Square index) {
    int pawn_shelter_value = 150;
    int middle = std::max(FILE_B, std::min(FILE_G, col(index)));

    int king_rank = rank(index, color);
    pawn_shelter_value -= (king_rank - RANK_1) * pawn_shelter_penalty[3]; // Take a look at this again sometime

    for (int column = middle - 1; column <= middle + 1; column++) {
        Bitboard pawns = p->bbs[pawn(color)] & FILE_MASK[column];
        if (pawns) {
            Square closest_pawn = color == white ? lsb(pawns) : msb(pawns);
            int pawn_rank = rank(closest_pawn, color);
            if (king_rank <= pawn_rank) {
                pawn_shelter_value -= pawn_shelter_penalty[pawn_rank - king_rank];
            }
        } else {
            pawn_shelter_value -= pawn_shelter_penalty[7];
        }
    }

    return pawn_shelter_value;
}

Score evaluate_king(Evaluation *eval, Position *p, Color color) {
    Score king_score = {0, 0};
    Color opp_c = opponent_color(color);
    Square outpost = p->king_index[color];
    Bitboard king_targets = generate_king_targets(outpost);

    int pawn_shelter_value = evaluate_pawn_shelter(p, color, outpost);
    if (p->castling & can_king_castle_mask[color]) {
        pawn_shelter_value = std::max(pawn_shelter_value, evaluate_pawn_shelter(p, color, relative_square(G1, color)));
    }
    if (p->castling & can_queen_castle_mask[color]) {
        pawn_shelter_value = std::max(pawn_shelter_value, evaluate_pawn_shelter(p, color, relative_square(C1, color)));
    }
    king_score.midgame += pawn_shelter_value;

    if (p->bbs[pawn(color)]) {
        int pawn_distance = 0;
        while (!(DISTANCE_RING[outpost][pawn_distance++] & p->bbs[pawn(color)])) {}
        king_score.endgame -= pawn_distance_penalty * pawn_distance;
    }

    if (eval->num_king_attackers[color] > (1 - eval->num_pieces[queen(opp_c)])) {
        // Attackers to the king extended zone
        int king_danger = -pawn_shelter_value / 10 - 48 * !eval->num_pieces[queen(opp_c)];

        Bitboard b = eval->targets[opp_c] & ~eval->targets[color] & eval->king_zone[color] & ~p->bbs[opp_c];
        king_danger += count(b) * 10;
        king_danger += eval->num_king_attackers[color] * eval->king_zone_score[color] / 12;
        king_danger += eval->num_king_zone_attacks[color] * king_zone_attack_penalty;

        if (p->pinned[color]) {
            king_danger += 6;
        }

        // Unprotected and attacked squares in the king zone
        Bitboard king_only_protected = king_targets & eval->targets[opp_c] & ~eval->double_targets[color];
        king_danger += king_only_protected_penalty * count(king_only_protected);

        // Safe squares that can be checked by the opponent
        Bitboard safe = ~p->bbs[opp_c] & (~eval->targets[color] | (king_only_protected & eval->double_targets[opp_c]));

        Bitboard rook_attacks = generate_rook_targets(p->board, outpost);
        Bitboard bishop_attacks = generate_bishop_targets(p->board, outpost);
        Bitboard knight_targets = generate_knight_targets(outpost);

        // Queen checks
        if ((rook_attacks | bishop_attacks) & eval->targets[queen(opp_c)] & safe) {
            king_danger += queen_check_penalty;
        }

        safe |= eval->double_targets[opp_c] & ~(eval->double_targets[color] | p->bbs[opp_c]) & eval->targets[queen(color)];

        if (rook_attacks & eval->targets[rook(opp_c)] & safe) {
            king_danger += rook_check_penalty;
        }
        if (bishop_attacks & eval->targets[bishop(opp_c)] & safe) {
            king_danger += bishop_check_penalty;
        }
        if (knight_targets & eval->targets[knight(opp_c)] & safe) {
            king_danger += knight_check_penalty;
        }

        if (king_danger > 0) {
            king_score -= Score{king_danger * king_danger / 32, king_danger / 2};
        }
    }

    return king_score;
}

Score evaluate_threat(Evaluation *eval, Position *p, Color color) {
    Score threat_score = {0, 0};
    Color opp_c = opponent_color(color);

    // Non pawn enemies that are attacked by pawns
    Bitboard opponent_non_pawns = p->bbs[opp_c] ^ p->bbs[pawn(opp_c)];
    Bitboard attacked_non_pawns = opponent_non_pawns & eval->targets[pawn(color)];

    if (attacked_non_pawns) {
        Bitboard safe_pawns = p->bbs[pawn(color)] & (~eval->targets[opp_c] | eval->targets[color]);
        Bitboard safe_threats = generate_pawn_threats(safe_pawns, color) & attacked_non_pawns;
        threat_score += strong_pawn_threat * count(safe_threats);

        if (attacked_non_pawns ^ safe_threats) {
            threat_score += weak_pawn_threat;
        }
    }

    Bitboard very_supported = eval->targets[pawn(opp_c)] | (eval->double_targets[opp_c] & ~eval->double_targets[color]);
    Bitboard supported_non_pawns = opponent_non_pawns & very_supported;
    Bitboard not_supported = p->bbs[opp_c] & ~very_supported & eval->targets[color];

    if (supported_non_pawns | not_supported) {
        Bitboard attacked_by_minor = (supported_non_pawns | not_supported) & (eval->targets[knight(color)] | eval->targets[bishop(color)]);
        while (attacked_by_minor) {
            Square outpost = pop(&attacked_by_minor);
            threat_score += minor_threat_bonus[piece_type(p->pieces[outpost])];
            if (!is_pawn(p->pieces[outpost]))
                threat_score += rank_threat_bonus * rank(outpost, opp_c);
        }

        Bitboard attacked_by_rook = (p->bbs[queen(opp_c)] | not_supported) & eval->targets[rook(color)];
        while (attacked_by_rook) {
            Square outpost = pop(&attacked_by_rook);
            threat_score += rook_threat_bonus[piece_type(p->pieces[outpost])];
            if (!is_pawn(p->pieces[outpost]))
                threat_score += rank_threat_bonus * rank(outpost, opp_c);
        }

        threat_score += hanging_threat_bonus * count(not_supported & ~eval->targets[opp_c]);
    }

    //? Pawn push threats
    Bitboard pawn_can_push  = (color == white ? p->bbs[pawn(color)] << 8 : p->bbs[pawn(color)] >> 8 ) & ~p->board;
    pawn_can_push          |= (color == white ? ((pawn_can_push & RANK_3BB) << 8) : ((pawn_can_push & RANK_6BB) >> 8)) & ~p->board;

    //? Pop unsafes
    pawn_can_push          &= ~eval->targets[pawn(opp_c)] & (eval->targets[color] | ~eval->targets[opp_c]);
    threat_score += pawn_push_threat_bonus * count(generate_pawn_threats(pawn_can_push, color) & p->bbs[opp_c] & ~eval->targets[pawn(color)]);

    return threat_score;
}

Score evaluate_passer(Evaluation *eval, Position *p, Color color) {
    Score passer_score = {0, 0};
    Color opp_c = opponent_color(color);
    Bitboard passers = eval->pawn_passers[color];

    while (passers) {
        Square outpost = pop(&passers);
        Bitboard hinder = FRONT_MASK[outpost][color] & (eval->targets[opp_c] | p->bbs[opp_c]);
        passer_score -= hindered_passer_penalty * count(hinder);

        int r = rank(outpost, color) - RANK_2;
        int rr = r * (r - 1) / 2;

        int midgame = passed_pawn_bonus[r].midgame;
        int endgame = passed_pawn_bonus[r].endgame;

        if (rr) {
            Square blocker = pawn_forward(outpost, color);

            // King proximity
            endgame += (distance(p->king_index[opp_c], blocker) * 5 -
                        distance(p->king_index[color], blocker) * 2) * rr;

            if (rank(blocker, color) != RANK_8)
                endgame -= distance(p->king_index[color], pawn_forward(blocker, color)) * rr;

            if (p->pieces[blocker] == no_piece) {
                Bitboard defended, unsafe, to_queen;
                defended = unsafe = to_queen = FRONT_MASK[outpost][color];

                Bitboard rook_queen_attacks = FRONT_MASK[outpost][opp_c] &
                                              (p->bbs[rook(color)] | p->bbs[queen(color)] | p->bbs[rook(opp_c)] | p->bbs[queen(opp_c)]) &
                                              generate_rook_targets(p->board, outpost);

                if (!(p->bbs[color] & rook_queen_attacks))
                    defended &= eval->targets[color];

                if (!(p->bbs[opp_c] & rook_queen_attacks))
                    unsafe &= eval->targets[opp_c] | p->bbs[opp_c];

                int bonus = !unsafe ? 18 : !(unsafe & bfi[blocker]) ? 8 : 0;

                if (defended == to_queen)
                    bonus += 6;
                else if (defended & bfi[blocker])
                    bonus += 4;

                midgame += bonus * rr;
                endgame += bonus * rr;
            } else if (p->bbs[color] & bfi[blocker]) {
                midgame += rr + r;
                endgame += rr + r;
            }
        }

        // Candidate passers get fewer points
        if ((PASSED_PAWN_MASK[outpost][color] & p->bbs[pawn(opp_c)]) || (FRONT_MASK[outpost][color] & p->bbs[pawn(color)])) {
            midgame /= 2;
            endgame /= 2;
        }

        passer_score += Score{midgame, endgame} + passed_file_bonus[col(outpost)];
    }

    return passer_score;
}

void evaluate_bishops(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_bishop(eval, p, white);
    eval->score_black += evaluate_bishop(eval, p, black);
}

void evaluate_knights(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_knight(eval, p, white);
    eval->score_black += evaluate_knight(eval, p, black);
}

void evaluate_rooks(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_rook(eval, p, white);
    eval->score_black += evaluate_rook(eval, p, black);
}

void evaluate_queens(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_queen(eval, p, white);
    eval->score_black += evaluate_queen(eval, p, black);
}

void evaluate_pieces(Evaluation *eval, Position *p) {
    evaluate_bishops(eval, p);
    evaluate_knights(eval, p);
    evaluate_rooks(eval, p);
    evaluate_queens(eval, p);
}

void evaluate_kings(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_king(eval, p, white);
    eval->score_black += evaluate_king(eval, p, black);
}

void evaluate_threats(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_threat(eval, p, white);
    eval->score_black += evaluate_threat(eval, p, black);
}

void evaluate_mobility(Evaluation *eval) {
    eval->score_white += eval->mobility_score[white];
    eval->score_black += eval->mobility_score[black];
}

void evaluate_passers(Evaluation *eval, Position *p) {
    eval->score_white += evaluate_passer(eval, p, white);
    eval->score_black += evaluate_passer(eval, p, black);
}

void pre_eval(Evaluation *eval, Position *p) {
    eval->targets[white_pawn] = generate_pawn_threats(p->bbs[white_pawn], white);
    eval->targets[black_pawn] = generate_pawn_threats(p->bbs[black_pawn], black);
    Bitboard white_king_targets = generate_king_targets(p->king_index[white]);
    Bitboard black_king_targets = generate_king_targets(p->king_index[black]);

    if (rank(p->king_index[white], white) == RANK_1) {
        eval->king_zone[white] = KING_EXTENDED_MASKS[white][p->king_index[white]];
    } else {
        eval->king_zone[white] = white_king_targets;
    }

    if (rank(p->king_index[black], black) == RANK_1) {
        eval->king_zone[black] = KING_EXTENDED_MASKS[black][p->king_index[black]];
    } else {
        eval->king_zone[black] = black_king_targets;
    }

    eval->double_targets[white] = white_king_targets & eval->targets[white_pawn];
    eval->double_targets[black] = black_king_targets & eval->targets[black_pawn];
    eval->targets[white] = white_king_targets | eval->targets[white_pawn];
    eval->targets[black] = black_king_targets | eval->targets[black_pawn];

    eval->semi_open_files[white] = 0xFF;
    eval->semi_open_files[black] = 0xFF;

    // Kings are pieces too (but no need for num_pieces[king(color)] since king is always there)
    eval->num_pieces[white] = 2;
    eval->num_pieces[black] = 2;

    Bitboard low_ranks_white = RANK_2BB | RANK_3BB;
    Bitboard b_white = p->bbs[pawn(white)] & ((p->board >> 8) | low_ranks_white);
    eval->mobility_area[white] = ~(b_white | p->bbs[king(white)] | eval->targets[black_pawn]);

    Bitboard low_ranks_black = RANK_6BB | RANK_7BB;
    Bitboard b_black = p->bbs[pawn(black)] & ((p->board << 8) | low_ranks_black);
    eval->mobility_area[black] = ~(b_black | p->bbs[king(black)] | eval->targets[white_pawn]);
}

int scaling_factor(Evaluation *eval, Position *p, Material *eval_material) {
    if (eval_material->scaling_factor_type != NO_SCALING) {
        return 4;  // This is KRPKR since we have no other endgames yet.
    }

    Color winner = p->score.endgame > 0 ? white : black;
    Color loser = opponent_color(winner);

    if (eval->num_pieces[pawn(winner)] <= 1 &&
        p->non_pawn_material[winner] <= p->non_pawn_material[loser] + piece_values[white_bishop]) {
            if (eval->num_pieces[pawn(winner)] == 0) {
                return 4;
            }
            return 24;
    }

    // Opposite bishops
    if (eval->num_pieces[white_bishop] == 1 && eval->num_pieces[black_bishop] == 1 &&
        opposite_colors(eval->bishop_squares[white], eval->bishop_squares[black])) {
        if (p->non_pawn_material[white] == BISHOP_MID && p->non_pawn_material[black] == BISHOP_MID) {
            return 16;
        } else {
            return 24;
        }
    }
    return SCALE_NORMAL;
}

int evaluate(Position *p) {
    assert(!is_checked(p));

    Evaluation eval = init_evaluation;
    pre_eval(&eval, p);

    Material *eval_material = get_material(p);
    EndgameType endgame_type = eval_material->endgame_type;
    if (endgame_type != NORMAL_ENDGAME) {
        if (endgame_type == KNOWN_ENDGAME_KPK && count(p->board) == 3) {
            return evaluate_kpk(p);
        } else if (endgame_type == KNOWN_ENDGAME_KXK && count(p->board) == 3) {
            return evaluate_kxk(p);
        } else if (endgame_type == DRAW_ENDGAME) {
            return 0;
        }
    }

    eval.score += p->score;

    evaluate_pawns(&eval, p);
    eval.score += eval.score_pawn;

    int early = (eval.score.midgame + eval.score.endgame) / 2;
    if (std::abs(early) > 880) {
        return p->color == white ? early : -early;
    }

    evaluate_pieces(&eval, p);
    evaluate_kings(&eval, p);
    evaluate_threats(&eval, p);
    evaluate_mobility(&eval);
    evaluate_passers(&eval, p);

    eval.score += (eval.score_white - eval.score_black);
    eval.score += eval_material->score;
    int scale = scaling_factor(&eval, p, eval_material);
    int ret = (eval.score.midgame * eval_material->phase + eval.score.endgame * (256 - eval_material->phase) * scale / SCALE_NORMAL) / 256;
    return (p->color == white ? ret : -ret) + tempo;
}

void print_eval(Position *p){
    Evaluation eval = init_evaluation;
    pre_eval(&eval, p);

    evaluate_pawns(&eval, p);
    Score Pawn_score = eval.score_pawn;

    Score Position_score = p->score;

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_knights(&eval, p);
    Score Knight_score[2] = {eval.score_white, eval.score_black};

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_bishops(&eval, p);
    Score Bishop_score[2] = {eval.score_white, eval.score_black};

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_rooks(&eval, p);
    Score Rook_score[2] = {eval.score_white, eval.score_black};

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_queens(&eval, p);
    Score Queen_score[2] = {eval.score_white, eval.score_black};

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_kings(&eval, p);
    Score King_score[2] = {eval.score_white, eval.score_black};

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_threats(&eval, p);
    Score Threat_score[2] = {eval.score_white, eval.score_black};

    Score Mobility_score[2] = {eval.mobility_score[white], eval.mobility_score[black]};

    Material *eval_material = get_material(p);

    eval.score_white = 0;
    eval.score_black = 0;
    evaluate_passers(&eval, p);
    Score Passer_score[2] = {eval.score_white, eval.score_black};

    Score blacky = Passer_score[black] + Mobility_score[black] + Knight_score[black] + Bishop_score[black] + Rook_score[black] + Queen_score[black] + King_score[black] + Threat_score[black];
    Score whitey = Passer_score[white] + Mobility_score[white] + Knight_score[white] + Bishop_score[white] + Rook_score[white] + Queen_score[white] + King_score[white] + Threat_score[white];


    std::cout << "  ATTRIBUTE   Score : " << "----WHITE----|" << "----BLACK----|" << "----TOTAL----" << std::endl;
    std::cout << "  Position    Score : " << "                            " << score_str(Position_score) << std::endl;
    std::cout << "  Imbalance   Score : " << "                            " << score_str(Score{0, 0} + eval_material->score) << std::endl;
    std::cout << "  Pawn        Score : " << "                            " << score_str(Pawn_score) << std::endl;
    std::cout << "  Passer      Score : " << score_str(Passer_score[white]) << "|" << score_str(Passer_score[black]) << "|" << score_str(Passer_score[white] - Passer_score[black]) << std::endl;
    std::cout << "  King        Score : " << score_str(King_score[white]) << "|" << score_str(King_score[black]) << "|" << score_str(King_score[white] - King_score[black]) << std::endl;
    std::cout << "  Threat      Score : " << score_str(Threat_score[white]) << "|" << score_str(Threat_score[black]) << "|" << score_str(Threat_score[white] - Threat_score[black]) << std::endl;
    std::cout << "  Mobility    Score : " << score_str(Mobility_score[white]) << "|" << score_str(Mobility_score[black]) << "|" << score_str(Mobility_score[white] - Mobility_score[black]) << std::endl;
    std::cout << "  Bishop      Score : " << score_str(Bishop_score[white]) << "|" << score_str(Bishop_score[black]) << "|" << score_str(Bishop_score[white] - Bishop_score[black]) << std::endl;
    std::cout << "  Knight      Score : " << score_str(Knight_score[white]) << "|" << score_str(Knight_score[black]) << "|" << score_str(Knight_score[white] - Knight_score[black]) << std::endl;
    std::cout << "  Rook        Score : " << score_str(Rook_score[white]) << "|" << score_str(Rook_score[black]) << "|" << score_str(Rook_score[white] - Rook_score[black]) << std::endl;
    std::cout << "  Queen       Score : " << score_str(Queen_score[white]) << "|" << score_str(Queen_score[black]) << "|" << score_str(Queen_score[white] - Queen_score[black]) << std::endl;

    std::cout << std::endl << "  Total       Score : " << score_str(whitey) << "|" << score_str(blacky) << "|" << score_str(eval.score_pawn + p->score + whitey - blacky + eval_material->score) << std::endl;
    int CP = 100 * p->color == white ? evaluate(p) : -evaluate(p);
    double percentage = (double)(256 - eval_material->phase) / 256.0 * 100.0;
    std::cout << std::endl;
    std::cout << "                                                   Phase: " << percentage << "%" << std::endl;
    std::cout << "                                                   Scale: " << scaling_factor(&eval, p, eval_material) << " / " << SCALE_NORMAL << std::endl;
    std::cout << "                                                   CP   : " << (double) CP / (double) PAWN_END << " (for white)" << std::endl;
}
