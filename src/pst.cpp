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

#include "pst.h"

void init_values() {
    for (int s = A1; s <= H8; s++) {
        int flip = s ^ A8;

        int flip_row = row(flip);
        int flip_col = col(flip);
        if (flip_col < 4) {
            flip = flip_row * 4 + flip_col;
        } else {
            flip = flip_row * 4 + 7 - flip_col;
        }

        int sq = s;
        int sq_row = row(sq);
        int sq_col = col(sq);
        if (sq_col < 4) {
            sq = sq_row * 4 + sq_col;
        } else {
            sq = sq_row * 4 + 7 - sq_col;
        }

        pst[white_pawn][s]   = Score{bonusPawn  [0][flip], bonusPawn  [1][flip]} + Score{PAWN_MID, PAWN_END};
        pst[white_knight][s] = Score{bonusKnight[0][flip], bonusKnight[1][flip]} + Score{KNIGHT_MID, KNIGHT_END};
        pst[white_bishop][s] = Score{bonusBishop[0][flip], bonusBishop[1][flip]} + Score{BISHOP_MID, BISHOP_END};
        pst[white_rook][s]   = Score{bonusRook  [0][flip], bonusRook  [1][flip]} + Score{ROOK_MID, ROOK_END};
        pst[white_queen][s]  = Score{bonusQueen [0][flip], bonusQueen [1][flip]} + Score{QUEEN_MID, QUEEN_END};
        pst[white_king][s]   = Score{bonusKing  [0][flip], bonusKing  [1][flip]} + Score{0, 0};

        pst[black_pawn][s]   = Score{-bonusPawn  [0][sq], -bonusPawn  [1][sq]} - Score{PAWN_MID, PAWN_END};
        pst[black_knight][s] = Score{-bonusKnight[0][sq], -bonusKnight[1][sq]} - Score{KNIGHT_MID, KNIGHT_END};
        pst[black_bishop][s] = Score{-bonusBishop[0][sq], -bonusBishop[1][sq]} - Score{BISHOP_MID, BISHOP_END};
        pst[black_rook][s]   = Score{-bonusRook  [0][sq], -bonusRook  [1][sq]} - Score{ROOK_MID, ROOK_END};
        pst[black_queen][s]  = Score{-bonusQueen [0][sq], -bonusQueen [1][sq]} - Score{QUEEN_MID, QUEEN_END};
        pst[black_king][s]   = Score{-bonusKing  [0][sq], -bonusKing  [1][sq]} - Score{0, 0};
    }

    piece_values[0] = 0;
    piece_values[1] = 0;
    piece_values[2] = PAWN_MID;
    piece_values[3] = PAWN_MID;
    piece_values[4] = KNIGHT_MID;
    piece_values[5] = KNIGHT_MID;
    piece_values[6] = BISHOP_MID;
    piece_values[7] = BISHOP_MID;
    piece_values[8] = ROOK_MID;
    piece_values[9] = ROOK_MID;
    piece_values[10] = QUEEN_MID;
    piece_values[11] = QUEEN_MID;
    piece_values[12] = QUEEN_MID * 100;
    piece_values[13] = QUEEN_MID * 100;

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 14; j++) {
            mvvlva_values[i][j] = piece_values[i] - j;
        }
    }
}
