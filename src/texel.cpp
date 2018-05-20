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
#include "pst.h"

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

int
    PAWN_MID = 100, PAWN_END = 140,
    KNIGHT_MID = 400, KNIGHT_END = 440,
    BISHOP_MID = 430, BISHOP_END = 470,
    ROOK_MID = 650, ROOK_END = 710,
    QUEEN_MID = 1300, QUEEN_END = 1350;

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
    fens.open("fewfens.txt");
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
    parameters.push_back({50, 200, &PAWN_MID, "PAWN_MID"});
    parameters.push_back({105, 200, &PAWN_END, "PAWN_END"});

    parameters.push_back({200, 800, &KNIGHT_MID, "KNIGHT_MID"});
    parameters.push_back({200, 800, &KNIGHT_END, "KNIGHT_END"});

    parameters.push_back({200, 800, &BISHOP_MID, "BISHOP_MID"});
    parameters.push_back({200, 800, &BISHOP_END, "BISHOP_END"});

    parameters.push_back({400, 1000, &ROOK_MID, "ROOK_MID"});
    parameters.push_back({400, 1000, &ROOK_END, "ROOK_END"});

    parameters.push_back({800, 2000, &QUEEN_MID, "QUEEN_MID"});
    parameters.push_back({800, 2000, &QUEEN_END, "QUEEN_END"});

    parameters.push_back({0, 100, &queen_check_penalty, "queen_check_penalty"});
    parameters.push_back({0, 100, &rook_check_penalty, "rook_check_penalty"});
    parameters.push_back({0, 100, &knight_check_penalty, "knight_check_penalty"});
    parameters.push_back({0, 100, &bishop_check_penalty, "bishop_check_penalty"});

    parameters.push_back({0, 30, &protected_piece_bonus.midgame, "protected_piece_bonus.midgame"});

    parameters.push_back({0, 20, &rook_pawn_bonus.midgame, "rook_pawn_bonus.midgame"});
    parameters.push_back({0, 40, &rook_pawn_bonus.endgame, "rook_pawn_bonus.endgame"});

    parameters.push_back({0, 40, &minor_piece_behind_pawn.midgame, "minor_piece_behind_pawn.midgame"});

    parameters.push_back({0, 200, &strong_pawn_threat.midgame, "strong_pawn_threat.midgame"});
    parameters.push_back({0, 200, &strong_pawn_threat.endgame, "strong_pawn_threat.endgame"});

    parameters.push_back({0, 100, &weak_pawn_threat.midgame, "weak_pawn_threat.midgame"});
    parameters.push_back({0, 100, &weak_pawn_threat.endgame, "weak_pawn_threat.endgame"});

    parameters.push_back({0, 30, &rank_threat_bonus.midgame, "rank_threat_bonus.midgame"});
    parameters.push_back({0, 10, &rank_threat_bonus.endgame, "rank_threat_bonus.endgame"});

    parameters.push_back({0, 60, &hanging_threat_bonus.midgame, "hanging_threat_bonus.midgame"});
    parameters.push_back({0, 30, &hanging_threat_bonus.endgame, "hanging_threat_bonus.endgame"});

    parameters.push_back({0, 40, &pawn_push_threat_bonus.midgame, "pawn_push_threat_bonus.midgame"});
    parameters.push_back({0, 30, &pawn_push_threat_bonus.endgame, "pawn_push_threat_bonus.endgame"});

    parameters.push_back({0, 20, &double_pawn_penalty.midgame, "double_pawn_penalty.midgame"});
    parameters.push_back({0, 40, &double_pawn_penalty.endgame, "double_pawn_penalty.endgame"});

    parameters.push_back({0, 150, &blocked_rook_penalty.midgame, "blocked_rook_penalty.midgame"});

    parameters.push_back({0, 20, &bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame"});
    parameters.push_back({0, 20, &bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame"});

    parameters.push_back({0, 20, &hindered_passer_penalty.midgame, "hindered_passer_penalty.midgame"});

    parameters.push_back({0, 20, &bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame"});
    parameters.push_back({0, 20, &bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame"});

    parameters.push_back({0, 40, &king_only_protected_penalty, "king_only_protected_penalty"});
    parameters.push_back({0, 120, &queen_check_penalty, "queen_check_penalty"});
    parameters.push_back({0, 120, &knight_check_penalty, "knight_check_penalty"});
    parameters.push_back({0, 120, &rook_check_penalty, "rook_check_penalty"});
    parameters.push_back({0, 120, &bishop_check_penalty, "bishop_check_penalty"});
    parameters.push_back({0, 20, &pawn_distance_penalty, "pawn_distance_penalty"});
    parameters.push_back({0, 20, &king_zone_attack_penalty, "king_zone_attack_penalty"});

    parameters.push_back({0, 20, &passed_pawn_bonus[0].midgame, "passed_pawn_bonus[0].midgame"});
    parameters.push_back({0, 20, &passed_pawn_bonus[0].endgame, "passed_pawn_bonus[0].endgame"});
    parameters.push_back({0, 30, &passed_pawn_bonus[1].midgame, "passed_pawn_bonus[1].midgame"});
    parameters.push_back({0, 30, &passed_pawn_bonus[1].endgame, "passed_pawn_bonus[1].endgame"});
    parameters.push_back({0, 40, &passed_pawn_bonus[2].midgame, "passed_pawn_bonus[2].midgame"});
    parameters.push_back({0, 40, &passed_pawn_bonus[2].endgame, "passed_pawn_bonus[2].endgame"});
    parameters.push_back({0, 100, &passed_pawn_bonus[3].midgame, "passed_pawn_bonus[3].midgame"});
    parameters.push_back({0, 100, &passed_pawn_bonus[3].endgame, "passed_pawn_bonus[3].endgame"});
    parameters.push_back({0, 200, &passed_pawn_bonus[4].midgame, "passed_pawn_bonus[4].midgame"});
    parameters.push_back({0, 200, &passed_pawn_bonus[4].endgame, "passed_pawn_bonus[4].endgame"});
    parameters.push_back({0, 300, &passed_pawn_bonus[5].midgame, "passed_pawn_bonus[5].midgame"});
    parameters.push_back({0, 300, &passed_pawn_bonus[5].endgame, "passed_pawn_bonus[5].endgame"});

    parameters.push_back({0, 30, &rook_file_bonus[0].midgame, "rook_file_bonus[0].midgame"});
    parameters.push_back({0, 10, &rook_file_bonus[0].endgame, "rook_file_bonus[0].endgame"});
    parameters.push_back({0, 40, &rook_file_bonus[1].midgame, "rook_file_bonus[1].midgame"});
    parameters.push_back({0, 30, &rook_file_bonus[1].endgame, "rook_file_bonus[1].endgame"});

    parameters.push_back({0, 40, &isolated_pawn_penalty[0].midgame, "isolated_pawn_penalty[0].midgame"});
    parameters.push_back({0, 40, &isolated_pawn_penalty[0].endgame, "isolated_pawn_penalty[0].endgame"});
    parameters.push_back({0, 20, &isolated_pawn_penalty[1].midgame, "isolated_pawn_penalty[1].midgame"});
    parameters.push_back({0, 20, &isolated_pawn_penalty[1].endgame, "isolated_pawn_penalty[1].endgame"});

    parameters.push_back({0, 40, &backward_pawn_penalty[0].midgame, "backward_pawn_penalty[0].midgame"});
    parameters.push_back({0, 40, &backward_pawn_penalty[0].endgame, "backward_pawn_penalty[0].endgame"});
    parameters.push_back({0, 20, &backward_pawn_penalty[1].midgame, "backward_pawn_penalty[1].midgame"});
    parameters.push_back({0, 20, &backward_pawn_penalty[1].endgame, "backward_pawn_penalty[1].endgame"});

    parameters.push_back({0, 20, &minor_threat_bonus[1].midgame, "minor_threat_bonus[1].midgame"});
    parameters.push_back({0, 20, &minor_threat_bonus[1].endgame, "minor_threat_bonus[1].endgame"});
    parameters.push_back({0, 40, &minor_threat_bonus[2].midgame, "minor_threat_bonus[2].midgame"});
    parameters.push_back({0, 40, &minor_threat_bonus[2].endgame, "minor_threat_bonus[2].endgame"});
    parameters.push_back({0, 40, &minor_threat_bonus[3].midgame, "minor_threat_bonus[3].midgame"});
    parameters.push_back({0, 40, &minor_threat_bonus[3].endgame, "minor_threat_bonus[3].endgame"});
    parameters.push_back({0, 80, &minor_threat_bonus[4].midgame, "minor_threat_bonus[4].midgame"});
    parameters.push_back({0, 80, &minor_threat_bonus[4].endgame, "minor_threat_bonus[4].endgame"});
    parameters.push_back({0, 80, &minor_threat_bonus[5].midgame, "minor_threat_bonus[5].midgame"});
    parameters.push_back({0, 80, &minor_threat_bonus[5].endgame, "minor_threat_bonus[5].endgame"});

    parameters.push_back({0, 20, &rook_threat_bonus[1].midgame, "rook_threat_bonus[1].midgame"});
    parameters.push_back({0, 20, &rook_threat_bonus[1].endgame, "rook_threat_bonus[1].endgame"});
    parameters.push_back({0, 60, &rook_threat_bonus[2].midgame, "rook_threat_bonus[2].midgame"});
    parameters.push_back({0, 70, &rook_threat_bonus[2].endgame, "rook_threat_bonus[2].endgame"});
    parameters.push_back({0, 60, &rook_threat_bonus[3].midgame, "rook_threat_bonus[3].midgame"});
    parameters.push_back({0, 70, &rook_threat_bonus[3].endgame, "rook_threat_bonus[3].endgame"});
    parameters.push_back({0, 20, &rook_threat_bonus[4].midgame, "rook_threat_bonus[4].midgame"});
    parameters.push_back({0, 20, &rook_threat_bonus[4].endgame, "rook_threat_bonus[4].endgame"});
    parameters.push_back({0, 60, &rook_threat_bonus[5].midgame, "rook_threat_bonus[5].midgame"});
    parameters.push_back({0, 70, &rook_threat_bonus[5].endgame, "rook_threat_bonus[5].endgame"});
}

void set_parameter(Parameter param, int value) {
    *param.variable = value;
    if (param.name == "PAWN_MID" || param.name == "PAWN_END" ||
        param.name == "KNIGHT_MID" || param.name == "KNIGHT_END" ||
        param.name == "BISHOP_MID" || param.name == "BISHOP_END" ||
        param.name == "ROOK_MID" || param.name == "ROOK_END" ||
        param.name == "QUEEN_MID" || param.name == "QUEEN_END"
    ) {
        init_pst();
    }
}

void tune() {
    init_parameters();
    for (unsigned i = 0; i < parameters.size(); ++i) {
        Parameter param = parameters[i];
        int initial_value = *param.variable;

        int min = param.min, max = param.max;
        int mid = 1 + (min + max) / 2;
        double k = 0.93;

        set_parameter(param, min);
        double min_err = find_error(k);
        cout << "errors[" << min << "]: " << min_err << endl;

        set_parameter(param, max);
        double max_err = find_error(k);
        cout << "errors[" << max << "]: " << max_err << endl;

        set_parameter(param, mid);
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
            set_parameter(param, mid);
            mid_err = find_error(k);
            cout << "errors[" << mid << "]: " << mid_err << endl;
        }
        cout << "best " << param.name << ": " << mid << endl;

        // Reset to initial value
        set_parameter(param, initial_value);
    }
}

