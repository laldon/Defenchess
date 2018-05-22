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

#ifdef __TUNE__

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
    int best;
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

mutex sum_mtx, file_mtx;
int n;
long double sum;
ifstream fens;
long double k = 0.93L;

long double sigmoid(long double s) {
    return 1.0L / (1.0L + pow(10.0L, -k * s / 400.0L));
}

void single_error(int thread_id) {
    string line;
    while (fens.good()) {
        file_mtx.lock();
        if (getline(fens, line)) {
            file_mtx.unlock();
            vector<string> fen_info;
            fen_split(line, fen_info);

            Position *p = import_fen(fen_info[0], thread_id);
            if (is_checked(p)) {
                continue;
            }

            Metadata *md = &p->my_thread->metadatas[0];
            md->current_move = no_move;
            md->static_eval = UNDEFINED;
            md->ply = 0;

            string result_str = fen_info[1];
            long double result;
            if (result_str == "1-0") {
                result = 1.0L;
            } else if (result_str == "0-1") {
                result = 0.0L;
            } else if (result_str == "1/2-1/2") {
                result = 0.5L;
            } else {
                exit(1);
                result = -1.0L;
            }
            int qi = alpha_beta_quiescence(p, md, -MATE, MATE, -1, false);
            qi = p->color == white ? qi : -qi;

            sum_mtx.lock();
            sum += pow(result - sigmoid((long double) qi), 2.0);
            ++n;
            sum_mtx.unlock();
        } else {
            file_mtx.unlock();
            break;
        }
    }
}

long double find_error() {
    n = 0;
    sum = 0.0L;
    fens.open("fewfens.txt");
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->thread_obj = std::thread(single_error, i);
    }
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = &search_threads[i];
        t->thread_obj.join();
    }
    fens.close();
    return sum / ((long double) (n));
}

void init_parameters() {
    parameters.push_back({50, 200, 0, &PAWN_MID, "PAWN_MID"});
    parameters.push_back({105, 200, 0, &PAWN_END, "PAWN_END"});

    parameters.push_back({200, 800, 0, &KNIGHT_MID, "KNIGHT_MID"});
    parameters.push_back({200, 800, 0, &KNIGHT_END, "KNIGHT_END"});

    parameters.push_back({200, 800, 0, &BISHOP_MID, "BISHOP_MID"});
    parameters.push_back({200, 800, 0, &BISHOP_END, "BISHOP_END"});

    parameters.push_back({400, 1000, 0, &ROOK_MID, "ROOK_MID"});
    parameters.push_back({400, 1000, 0, &ROOK_END, "ROOK_END"});

    parameters.push_back({800, 2000, 0, &QUEEN_MID, "QUEEN_MID"});
    parameters.push_back({800, 2000, 0, &QUEEN_END, "QUEEN_END"});

    parameters.push_back({0, 30, 0, &protected_piece_bonus.midgame, "protected_piece_bonus.midgame"});

    parameters.push_back({0, 40, 0, &rook_pawn_bonus.midgame, "rook_pawn_bonus.midgame"});
    parameters.push_back({0, 40, 0, &rook_pawn_bonus.endgame, "rook_pawn_bonus.endgame"});

    parameters.push_back({0, 40, 0, &minor_piece_behind_pawn.midgame, "minor_piece_behind_pawn.midgame"});

    parameters.push_back({0, 200, 0, &strong_pawn_threat.midgame, "strong_pawn_threat.midgame"});
    parameters.push_back({0, 200, 0, &strong_pawn_threat.endgame, "strong_pawn_threat.endgame"});

    parameters.push_back({0, 100, 0, &weak_pawn_threat.midgame, "weak_pawn_threat.midgame"});
    parameters.push_back({0, 100, 0, &weak_pawn_threat.endgame, "weak_pawn_threat.endgame"});

    parameters.push_back({0, 30, 0, &rank_threat_bonus.midgame, "rank_threat_bonus.midgame"});
    parameters.push_back({0, 10, 0, &rank_threat_bonus.endgame, "rank_threat_bonus.endgame"});

    parameters.push_back({0, 60, 0, &hanging_threat_bonus.midgame, "hanging_threat_bonus.midgame"});
    parameters.push_back({0, 30, 0, &hanging_threat_bonus.endgame, "hanging_threat_bonus.endgame"});

    parameters.push_back({0, 40, 0, &pawn_push_threat_bonus.midgame, "pawn_push_threat_bonus.midgame"});
    parameters.push_back({0, 30, 0, &pawn_push_threat_bonus.endgame, "pawn_push_threat_bonus.endgame"});

    parameters.push_back({0, 20, 0, &double_pawn_penalty.midgame, "double_pawn_penalty.midgame"});
    parameters.push_back({0, 40, 0, &double_pawn_penalty.endgame, "double_pawn_penalty.endgame"});

    parameters.push_back({0, 150, 0, &blocked_rook_penalty.midgame, "blocked_rook_penalty.midgame"});

    parameters.push_back({0, 20, 0, &bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame"});
    parameters.push_back({0, 20, 0, &bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame"});

    parameters.push_back({0, 20, 0, &hindered_passer_penalty.midgame, "hindered_passer_penalty.midgame"});

    parameters.push_back({0, 20, 0, &bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame"});
    parameters.push_back({0, 20, 0, &bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame"});

    parameters.push_back({0, 40, 0, &king_only_protected_penalty, "king_only_protected_penalty"});
    parameters.push_back({0, 120, 0, &queen_check_penalty, "queen_check_penalty"});
    parameters.push_back({0, 120, 0, &knight_check_penalty, "knight_check_penalty"});
    parameters.push_back({0, 120, 0, &rook_check_penalty, "rook_check_penalty"});
    parameters.push_back({0, 120, 0, &bishop_check_penalty, "bishop_check_penalty"});
    parameters.push_back({0, 20, 0, &pawn_distance_penalty, "pawn_distance_penalty"});
    parameters.push_back({0, 20, 0, &king_zone_attack_penalty, "king_zone_attack_penalty"});

    parameters.push_back({0, 20, 0, &passed_pawn_bonus[0].midgame, "passed_pawn_bonus[0].midgame"});
    parameters.push_back({0, 20, 0, &passed_pawn_bonus[0].endgame, "passed_pawn_bonus[0].endgame"});
    parameters.push_back({0, 30, 0, &passed_pawn_bonus[1].midgame, "passed_pawn_bonus[1].midgame"});
    parameters.push_back({0, 30, 0, &passed_pawn_bonus[1].endgame, "passed_pawn_bonus[1].endgame"});
    parameters.push_back({0, 40, 0, &passed_pawn_bonus[2].midgame, "passed_pawn_bonus[2].midgame"});
    parameters.push_back({0, 40, 0, &passed_pawn_bonus[2].endgame, "passed_pawn_bonus[2].endgame"});
    parameters.push_back({0, 100, 0, &passed_pawn_bonus[3].midgame, "passed_pawn_bonus[3].midgame"});
    parameters.push_back({0, 100, 0, &passed_pawn_bonus[3].endgame, "passed_pawn_bonus[3].endgame"});
    parameters.push_back({0, 200, 0, &passed_pawn_bonus[4].midgame, "passed_pawn_bonus[4].midgame"});
    parameters.push_back({0, 200, 0, &passed_pawn_bonus[4].endgame, "passed_pawn_bonus[4].endgame"});
    parameters.push_back({0, 300, 0, &passed_pawn_bonus[5].midgame, "passed_pawn_bonus[5].midgame"});
    parameters.push_back({0, 300, 0, &passed_pawn_bonus[5].endgame, "passed_pawn_bonus[5].endgame"});

    parameters.push_back({0, 30, 0, &rook_file_bonus[0].midgame, "rook_file_bonus[0].midgame"});
    parameters.push_back({0, 10, 0, &rook_file_bonus[0].endgame, "rook_file_bonus[0].endgame"});
    parameters.push_back({0, 40, 0, &rook_file_bonus[1].midgame, "rook_file_bonus[1].midgame"});
    parameters.push_back({0, 30, 0, &rook_file_bonus[1].endgame, "rook_file_bonus[1].endgame"});

    parameters.push_back({0, 40, 0, &isolated_pawn_penalty[0].midgame, "isolated_pawn_penalty[0].midgame"});
    parameters.push_back({0, 40, 0, &isolated_pawn_penalty[0].endgame, "isolated_pawn_penalty[0].endgame"});
    parameters.push_back({0, 20, 0, &isolated_pawn_penalty[1].midgame, "isolated_pawn_penalty[1].midgame"});
    parameters.push_back({0, 20, 0, &isolated_pawn_penalty[1].endgame, "isolated_pawn_penalty[1].endgame"});

    parameters.push_back({0, 40, 0, &backward_pawn_penalty[0].midgame, "backward_pawn_penalty[0].midgame"});
    parameters.push_back({0, 40, 0, &backward_pawn_penalty[0].endgame, "backward_pawn_penalty[0].endgame"});
    parameters.push_back({0, 20, 0, &backward_pawn_penalty[1].midgame, "backward_pawn_penalty[1].midgame"});
    parameters.push_back({0, 20, 0, &backward_pawn_penalty[1].endgame, "backward_pawn_penalty[1].endgame"});

    parameters.push_back({0, 20, 0, &minor_threat_bonus[1].midgame, "minor_threat_bonus[1].midgame"});
    parameters.push_back({0, 20, 0, &minor_threat_bonus[1].endgame, "minor_threat_bonus[1].endgame"});
    parameters.push_back({0, 40, 0, &minor_threat_bonus[2].midgame, "minor_threat_bonus[2].midgame"});
    parameters.push_back({0, 40, 0, &minor_threat_bonus[2].endgame, "minor_threat_bonus[2].endgame"});
    parameters.push_back({0, 40, 0, &minor_threat_bonus[3].midgame, "minor_threat_bonus[3].midgame"});
    parameters.push_back({0, 40, 0, &minor_threat_bonus[3].endgame, "minor_threat_bonus[3].endgame"});
    parameters.push_back({0, 80, 0, &minor_threat_bonus[4].midgame, "minor_threat_bonus[4].midgame"});
    parameters.push_back({0, 80, 0, &minor_threat_bonus[4].endgame, "minor_threat_bonus[4].endgame"});
    parameters.push_back({0, 80, 0, &minor_threat_bonus[5].midgame, "minor_threat_bonus[5].midgame"});
    parameters.push_back({0, 80, 0, &minor_threat_bonus[5].endgame, "minor_threat_bonus[5].endgame"});

    parameters.push_back({0, 20, 0, &rook_threat_bonus[1].midgame, "rook_threat_bonus[1].midgame"});
    parameters.push_back({0, 20, 0, &rook_threat_bonus[1].endgame, "rook_threat_bonus[1].endgame"});
    parameters.push_back({0, 60, 0, &rook_threat_bonus[2].midgame, "rook_threat_bonus[2].midgame"});
    parameters.push_back({0, 70, 0, &rook_threat_bonus[2].endgame, "rook_threat_bonus[2].endgame"});
    parameters.push_back({0, 60, 0, &rook_threat_bonus[3].midgame, "rook_threat_bonus[3].midgame"});
    parameters.push_back({0, 70, 0, &rook_threat_bonus[3].endgame, "rook_threat_bonus[3].endgame"});
    parameters.push_back({0, 20, 0, &rook_threat_bonus[4].midgame, "rook_threat_bonus[4].midgame"});
    parameters.push_back({0, 20, 0, &rook_threat_bonus[4].endgame, "rook_threat_bonus[4].endgame"});
    parameters.push_back({0, 60, 0, &rook_threat_bonus[5].midgame, "rook_threat_bonus[5].midgame"});
    parameters.push_back({0, 70, 0, &rook_threat_bonus[5].endgame, "rook_threat_bonus[5].endgame"});
}

void set_parameter(Parameter *param, int value) {
    *param->variable = value;
    if (param->name == "PAWN_MID" || param->name == "PAWN_END" ||
        param->name == "KNIGHT_MID" || param->name == "KNIGHT_END" ||
        param->name == "BISHOP_MID" || param->name == "BISHOP_END" ||
        param->name == "ROOK_MID" || param->name == "ROOK_END" ||
        param->name == "QUEEN_MID" || param->name == "QUEEN_END"
    ) {
        init_pst();
    }
}

long double errors[10000];

int find_min_error(Parameter *param) {
    // Clear errors
    for (int i = 0; i < 10000; ++i) {
        errors[i] = -2.0L;
    }

    int min = param->min, max = param->max;

    set_parameter(param, min);
    long double min_err = find_error();
    errors[min] = min_err;
    cout << "errors[" << min << "]:\t" << min_err << endl;

    set_parameter(param, max);
    long double max_err = find_error();
    errors[max] = max_err;
    cout << "errors[" << max << "]:\t" << max_err << endl;

    while (max > min) {
        if (min_err < max_err) {
            if (min == max - 1) {
                return min;
            }
            max = min + (max - min) / 2;
            set_parameter(param, max);
            if (errors[max] < -1.0L) {
                errors[max] = find_error();
            }
            max_err = errors[max];
            cout << "errors[" << max << "]:\t" << max_err << endl;
        } else {
            if (min == max - 1) {
                return max;
            }
            min = min + (max - min) / 2;
            set_parameter(param, min);
            if (errors[min] < -1.0) {
                errors[min] = find_error();
            }
            min_err = errors[min];
            cout << "errors[" << min << "]:\t" << min_err << endl;
        }
    }
    return min;
}

void find_best_k() {
    // Clear errors
    for (int i = 0; i < 10000; ++i) {
        errors[i] = -2.0L;
    }

    int min = 80, max = 120;

    k = (long double)(min) / 100.0L;
    long double min_err = find_error();
    errors[min] = min_err;
    cout << "errors[" << min << "]:\t" << min_err << endl;

    k = (long double)(max) / 100.0L;
    long double max_err = find_error();
    errors[max] = max_err;
    cout << "errors[" << max << "]:\t" << max_err << endl;

    while (max > min) {
        if (min_err < max_err) {
            if (min == max - 1) {
                k = (long double)(min) / 100.0L;
                return;
            }
            max = min + (max - min) / 2;
            k = (long double)(max) / 100.0L;
            if (errors[max] < -1.0L) {
                errors[max] = find_error();
            }
            max_err = errors[max];
            cout << "errors[" << max << "]:\t" << max_err << endl;
        } else {
            if (min == max - 1) {
                k = (long double)(max) / 100.0L;
                return;
            }
            min = min + (max - min) / 2;
            k = (long double)(min) / 100.0L;
            if (errors[min] < -1.0) {
                errors[min] = find_error();
            }
            min_err = errors[min];
            cout << "errors[" << min << "]:\t" << min_err << endl;
        }
    }
}

void tune() {
    cout.precision(32);
    init_parameters();
    int iterations = 2;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        find_best_k();
        cout << "best k: " << k << endl;
        for (unsigned i = 0; i < parameters.size(); ++i) {
            Parameter *param = &parameters[i];
            int optimal = find_min_error(param);
            param->best = optimal;
            cout << "best " << param->name << ": " << optimal << endl;
        }

        for (unsigned i = 0; i < parameters.size(); ++i) {
            Parameter *param = &parameters[i];
            cout << "best " << param->name << "(" << iteration << "): " << param->best << endl;
        }
    }
}

#endif
