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

#include "texel.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include "search.h"
#include "position.h"
#include "tt.h"
#include <mutex>

using namespace std;

typedef struct Parameter {
    int min;
    int max;
    int *variable;
    string name;
} Parameter;

vector<Parameter> parameters;

void fen_split(string s, vector<string> &f) {
    unsigned l_index = 0;
    for (unsigned i = 0 ; i < s.length() ; i++) {
        if (s[i] == '|') {
            f.push_back(s.substr(l_index, i - l_index));
            l_index = i + 1;
        }
        if (i == s.length() - 1) {
            f.push_back(s.substr(l_index));
        }
    }
}

Score
    protected_piece_bonus = {10, 0},
    rook_pawn_bonus = {5, 15},
    minor_piece_behind_pawn = {10, 0},
    strong_pawn_threat = {100, 100},
    weak_pawn_threat = {40, 40},
    rank_threat_bonus = {10, 2},
    hanging_threat_bonus = {30, 15},
    pawn_push_threat_bonus = {20, 15};

// Penalties
Score
    double_pawn_penalty = {10, 20},
    blocked_rook_penalty = {70, 0},
    bishop_pawn_penalty = {5, 5},
    hindered_passer_penalty = {5, 0};

int
    king_only_protected_penalty = 11,
    queen_check_penalty = 50,
    knight_check_penalty = 50,
    rook_check_penalty = 55,
    bishop_check_penalty = 30,
    pawn_distance_penalty = 10,
    king_zone_attack_penalty = 6;

Score mobility_bonus[4][32] = {};

Score passed_pawn_bonus[7] = {
    {3, 3}, {5, 5}, {20, 20}, {45, 45}, {100, 100}, {150, 150} // Pawn is never on RANK_8
};

Score passed_file_bonus[8] = {
    {5, 5}, {1, 5}, {0, -4}, {-11, -7}, {-11, -7}, {0, -4}, {1, 5}, {5, 5}
};

Score rook_file_bonus[2] = {{12, 4}, {26, 12}};

Score isolated_pawn_penalty[2] = {{16, 18}, {8, 11}},
            backward_pawn_penalty[2] = {{23, 15}, {14, 7}};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0, 10}, // Pawn
    {20, 20}, // Knight
    {20, 20}, // Bishop
    {30, 40}, // Rook
    {30, 40}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0, 10}, // Pawn
    {20, 35}, // Knight
    {20, 35}, // Bishop
    { 0,  0}, // Rook
    {20, 30}  // Queen
    // { 0, 0}  // King should never be called
};

double sigmoid(double s, double k) {
    return 1.0 / (1.0 + pow(10.0, -k * s / 400.0));
}

mutex sum_mtx, file_mtx;
int n;
double sum;
ifstream fens;

void single_error(int thread_id, double k) {
    string line;
    while (fens.good()) {
        file_mtx.lock();
        if (getline(fens, line)) {
            file_mtx.unlock();
            vector<string> fen_info;
            fen_split(line, fen_info);
            Position *p = import_fen(fen_info[0], thread_id);
            Metadata *md = &p->my_thread->metadatas[0];
            string result_str = fen_info[1];
            double result;
            if (result_str == "1-0") {
                result = 1.0;
            } else if (result_str == "0-1") {
                result = 0.0;
            } else if (result_str == "1/2-1/2") {
                result = 0.5;
            } else {
                assert(false);
                result = -1.0;
            }
            double qi = alpha_beta_quiescence(p, md, -MATE, MATE, -1, is_checked(p));
            qi = p->color == white ? qi : -qi;

            sum_mtx.lock();
            sum += pow(result - sigmoid(qi, k), 2);
            ++n;
            sum_mtx.unlock();
        } else {
            file_mtx.unlock();
            break;
        }
    }
}

double find_error(double k) {
    n = 0;
    sum = 0.0;
    fens.open("allfens.txt");
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->thread_obj = std::thread(single_error, i, k);
    }
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->thread_obj.join();
    }
    fens.close();
    return sum / double(n);
}

void init_parameters() {
    parameters.push_back({0, 100, &queen_check_penalty, "queen_check_penalty"});
    parameters.push_back({0, 100, &rook_check_penalty, "rook_check_penalty"});
    parameters.push_back({0, 100, &knight_check_penalty, "knight_check_penalty"});
    parameters.push_back({0, 100, &bishop_check_penalty, "bishop_check_penalty"});
}

void tune() {
    init_parameters();
    for (unsigned i = 0; i < parameters.size(); ++i) {
        Parameter param = parameters[i];
        int min = param.min, max = param.max;
        int mid = 1 + (min + max) / 2;
        double k = 0.93;

        *param.variable = min;
        double min_err = find_error(k);
        cout << "errors[" << min << "]: " << min_err << endl;

        *param.variable = max;
        double max_err = find_error(k);
        cout << "errors[" << max << "]: " << max_err << endl;

        *param.variable = mid;
        double mid_err = find_error(k);
        cout << "errors[" << mid << "]: " << mid_err << endl;

        while (true) {
            if (mid == min || mid == max) {
                break;
            }
            if (min_err < max_err) {
                max = mid;
                max_err = mid_err;
                mid = 1 + (min + max) / 2;
            } else if (max_err < min_err) {
                min = mid;
                min_err = mid_err;
                mid = 1 + (min + max) / 2;
            }
            *param.variable = mid;
            mid_err = find_error(k);
            cout << "errors[" << mid << "]: " << mid_err << endl;
        }
        cout << "best " << param.name << ": " << mid << endl;
    }
}

