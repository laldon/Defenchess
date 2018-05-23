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
    int *variable;
    int value;
    string name;
    bool increasing;
} Parameter;

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
vector<long double> diffs;
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
            diffs.push_back(pow(result - sigmoid((long double) qi), 2.0L));
            ++n;
            sum_mtx.unlock();
        } else {
            file_mtx.unlock();
            break;
        }
    }
}

void set_parameter(Parameter *param) {
    *param->variable = param->value;
    if (param->name == "PAWN_MID" || param->name == "PAWN_END" ||
        param->name == "KNIGHT_MID" || param->name == "KNIGHT_END" ||
        param->name == "BISHOP_MID" || param->name == "BISHOP_END" ||
        param->name == "ROOK_MID" || param->name == "ROOK_END" ||
        param->name == "QUEEN_MID" || param->name == "QUEEN_END"
    ) {
        init_pst();
    }
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
long double kahansum() {
    long double sum, c, y, t;
    sum = 0.0L;
    c = 0.0L;
    for (unsigned i = 0; i < diffs.size(); ++i) {
        y = diffs[i] - c;
        t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    return sum;
}
#pragma GCC pop_options

long double find_error(vector<Parameter> params) {
    n = 0;
    diffs.clear();
    for (unsigned i = 0; i < params.size(); ++i) {
        Parameter *param = &params[i];
        set_parameter(param);
    }
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
    return kahansum() / ((long double) (n));
}

void init_parameters(vector<Parameter> &parameters) {
    // DO NOT TUNE THIS parameters.push_back({&PAWN_MID, PAWN_MID, "PAWN_MID", true});
    parameters.push_back({&PAWN_END, PAWN_END, "PAWN_END", true});

    parameters.push_back({&KNIGHT_MID, KNIGHT_MID, "KNIGHT_MID", true});
    parameters.push_back({&KNIGHT_END, KNIGHT_END, "KNIGHT_END", true});

    parameters.push_back({&BISHOP_MID, BISHOP_MID, "BISHOP_MID", true});
    parameters.push_back({&BISHOP_END, BISHOP_END, "BISHOP_END", true});

    parameters.push_back({&ROOK_MID, ROOK_MID, "ROOK_MID", true});
    parameters.push_back({&ROOK_END, ROOK_END, "ROOK_END", true});

    parameters.push_back({&QUEEN_MID, QUEEN_MID, "QUEEN_MID", true});
    parameters.push_back({&QUEEN_END, QUEEN_END, "QUEEN_END", true});

    parameters.push_back({&protected_piece_bonus.midgame, protected_piece_bonus.midgame, "protected_piece_bonus.midgame", true});

    parameters.push_back({&rook_pawn_bonus.midgame, rook_pawn_bonus.midgame, "rook_pawn_bonus.midgame", true});
    parameters.push_back({&rook_pawn_bonus.endgame, rook_pawn_bonus.endgame, "rook_pawn_bonus.endgame", true});

    parameters.push_back({&minor_piece_behind_pawn.midgame, minor_piece_behind_pawn.midgame, "minor_piece_behind_pawn.midgame", true});

    parameters.push_back({&strong_pawn_threat.midgame, strong_pawn_threat.midgame, "strong_pawn_threat.midgame", true});
    parameters.push_back({&strong_pawn_threat.endgame, strong_pawn_threat.endgame, "strong_pawn_threat.endgame", true});

    // TUNE THESE TWO LATER, THEY'RE BUGGED
    // parameters.push_back({&weak_pawn_threat.midgame, weak_pawn_threat.midgame, "weak_pawn_threat.midgame", true});
    // parameters.push_back({&weak_pawn_threat.endgame, weak_pawn_threat.endgame, "weak_pawn_threat.endgame", true});

    parameters.push_back({&rank_threat_bonus.midgame, rank_threat_bonus.midgame, "rank_threat_bonus.midgame", true});
    parameters.push_back({&rank_threat_bonus.endgame, rank_threat_bonus.endgame, "rank_threat_bonus.endgame", true});

    parameters.push_back({&hanging_threat_bonus.midgame, hanging_threat_bonus.midgame, "hanging_threat_bonus.midgame", true});
    parameters.push_back({&hanging_threat_bonus.endgame, hanging_threat_bonus.endgame, "hanging_threat_bonus.endgame", true});

    parameters.push_back({&pawn_push_threat_bonus.midgame, pawn_push_threat_bonus.midgame, "pawn_push_threat_bonus.midgame", true});
    parameters.push_back({&pawn_push_threat_bonus.endgame, pawn_push_threat_bonus.endgame, "pawn_push_threat_bonus.endgame", true});

    parameters.push_back({&double_pawn_penalty.midgame, double_pawn_penalty.midgame, "double_pawn_penalty.midgame", true});
    parameters.push_back({&double_pawn_penalty.endgame, double_pawn_penalty.endgame, "double_pawn_penalty.endgame", true});

    parameters.push_back({&blocked_rook_penalty.midgame, blocked_rook_penalty.midgame, "blocked_rook_penalty.midgame", true});

    parameters.push_back({&bishop_pawn_penalty.midgame, bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame", true});
    parameters.push_back({&bishop_pawn_penalty.endgame, bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame", true});

    parameters.push_back({&hindered_passer_penalty.midgame, hindered_passer_penalty.midgame, "hindered_passer_penalty.midgame", true});

    parameters.push_back({&bishop_pawn_penalty.midgame, bishop_pawn_penalty.midgame, "bishop_pawn_penalty.midgame", true});
    parameters.push_back({&bishop_pawn_penalty.endgame, bishop_pawn_penalty.endgame, "bishop_pawn_penalty.endgame", true});

    parameters.push_back({&king_only_protected_penalty, king_only_protected_penalty, "king_only_protected_penalty", true});
    parameters.push_back({&queen_check_penalty, queen_check_penalty, "queen_check_penalty", true});
    parameters.push_back({&knight_check_penalty, knight_check_penalty, "knight_check_penalty", true});
    parameters.push_back({&rook_check_penalty, rook_check_penalty, "rook_check_penalty", true});
    parameters.push_back({&bishop_check_penalty, bishop_check_penalty, "bishop_check_penalty", true});
    parameters.push_back({&pawn_distance_penalty, pawn_distance_penalty, "pawn_distance_penalty", true});
    parameters.push_back({&king_zone_attack_penalty, king_zone_attack_penalty, "king_zone_attack_penalty", true});

    parameters.push_back({&passed_pawn_bonus[0].midgame, passed_pawn_bonus[0].midgame, "passed_pawn_bonus[0].midgame", true});
    parameters.push_back({&passed_pawn_bonus[0].endgame, passed_pawn_bonus[0].endgame, "passed_pawn_bonus[0].endgame", true});
    parameters.push_back({&passed_pawn_bonus[1].midgame, passed_pawn_bonus[1].midgame, "passed_pawn_bonus[1].midgame", true});
    parameters.push_back({&passed_pawn_bonus[1].endgame, passed_pawn_bonus[1].endgame, "passed_pawn_bonus[1].endgame", true});
    parameters.push_back({&passed_pawn_bonus[2].midgame, passed_pawn_bonus[2].midgame, "passed_pawn_bonus[2].midgame", true});
    parameters.push_back({&passed_pawn_bonus[2].endgame, passed_pawn_bonus[2].endgame, "passed_pawn_bonus[2].endgame", true});
    parameters.push_back({&passed_pawn_bonus[3].midgame, passed_pawn_bonus[3].midgame, "passed_pawn_bonus[3].midgame", true});
    parameters.push_back({&passed_pawn_bonus[3].endgame, passed_pawn_bonus[3].endgame, "passed_pawn_bonus[3].endgame", true});
    parameters.push_back({&passed_pawn_bonus[4].midgame, passed_pawn_bonus[4].midgame, "passed_pawn_bonus[4].midgame", true});
    parameters.push_back({&passed_pawn_bonus[4].endgame, passed_pawn_bonus[4].endgame, "passed_pawn_bonus[4].endgame", true});
    parameters.push_back({&passed_pawn_bonus[5].midgame, passed_pawn_bonus[5].midgame, "passed_pawn_bonus[5].midgame", true});
    parameters.push_back({&passed_pawn_bonus[5].endgame, passed_pawn_bonus[5].endgame, "passed_pawn_bonus[5].endgame", true});

    parameters.push_back({&rook_file_bonus[0].midgame, rook_file_bonus[0].midgame, "rook_file_bonus[0].midgame", true});
    parameters.push_back({&rook_file_bonus[0].endgame, rook_file_bonus[0].endgame, "rook_file_bonus[0].endgame", true});
    parameters.push_back({&rook_file_bonus[1].midgame, rook_file_bonus[1].midgame, "rook_file_bonus[1].midgame", true});
    parameters.push_back({&rook_file_bonus[1].endgame, rook_file_bonus[1].endgame, "rook_file_bonus[1].endgame", true});

    parameters.push_back({&isolated_pawn_penalty[0].midgame, isolated_pawn_penalty[0].midgame, "isolated_pawn_penalty[0].midgame", true});
    parameters.push_back({&isolated_pawn_penalty[0].endgame, isolated_pawn_penalty[0].endgame, "isolated_pawn_penalty[0].endgame", true});
    parameters.push_back({&isolated_pawn_penalty[1].midgame, isolated_pawn_penalty[1].midgame, "isolated_pawn_penalty[1].midgame", true});
    parameters.push_back({&isolated_pawn_penalty[1].endgame, isolated_pawn_penalty[1].endgame, "isolated_pawn_penalty[1].endgame", true});

    parameters.push_back({&backward_pawn_penalty[0].midgame, backward_pawn_penalty[0].midgame, "backward_pawn_penalty[0].midgame", true});
    parameters.push_back({&backward_pawn_penalty[0].endgame, backward_pawn_penalty[0].endgame, "backward_pawn_penalty[0].endgame", true});
    parameters.push_back({&backward_pawn_penalty[1].midgame, backward_pawn_penalty[1].midgame, "backward_pawn_penalty[1].midgame", true});
    parameters.push_back({&backward_pawn_penalty[1].endgame, backward_pawn_penalty[1].endgame, "backward_pawn_penalty[1].endgame", true});

    parameters.push_back({&minor_threat_bonus[1].midgame, minor_threat_bonus[1].midgame, "minor_threat_bonus[1].midgame", true});
    parameters.push_back({&minor_threat_bonus[1].endgame, minor_threat_bonus[1].endgame, "minor_threat_bonus[1].endgame", true});
    parameters.push_back({&minor_threat_bonus[2].midgame, minor_threat_bonus[2].midgame, "minor_threat_bonus[2].midgame", true});
    parameters.push_back({&minor_threat_bonus[2].endgame, minor_threat_bonus[2].endgame, "minor_threat_bonus[2].endgame", true});
    parameters.push_back({&minor_threat_bonus[3].midgame, minor_threat_bonus[3].midgame, "minor_threat_bonus[3].midgame", true});
    parameters.push_back({&minor_threat_bonus[3].endgame, minor_threat_bonus[3].endgame, "minor_threat_bonus[3].endgame", true});
    parameters.push_back({&minor_threat_bonus[4].midgame, minor_threat_bonus[4].midgame, "minor_threat_bonus[4].midgame", true});
    parameters.push_back({&minor_threat_bonus[4].endgame, minor_threat_bonus[4].endgame, "minor_threat_bonus[4].endgame", true});
    parameters.push_back({&minor_threat_bonus[5].midgame, minor_threat_bonus[5].midgame, "minor_threat_bonus[5].midgame", true});
    parameters.push_back({&minor_threat_bonus[5].endgame, minor_threat_bonus[5].endgame, "minor_threat_bonus[5].endgame", true});

    parameters.push_back({&rook_threat_bonus[1].midgame, rook_threat_bonus[1].midgame, "rook_threat_bonus[1].midgame", true});
    parameters.push_back({&rook_threat_bonus[1].endgame, rook_threat_bonus[1].endgame, "rook_threat_bonus[1].endgame", true});
    parameters.push_back({&rook_threat_bonus[2].midgame, rook_threat_bonus[2].midgame, "rook_threat_bonus[2].midgame", true});
    parameters.push_back({&rook_threat_bonus[2].endgame, rook_threat_bonus[2].endgame, "rook_threat_bonus[2].endgame", true});
    parameters.push_back({&rook_threat_bonus[3].midgame, rook_threat_bonus[3].midgame, "rook_threat_bonus[3].midgame", true});
    parameters.push_back({&rook_threat_bonus[3].endgame, rook_threat_bonus[3].endgame, "rook_threat_bonus[3].endgame", true});
    // No rook threat bonus 4
    parameters.push_back({&rook_threat_bonus[5].midgame, rook_threat_bonus[5].midgame, "rook_threat_bonus[5].midgame", true});
    parameters.push_back({&rook_threat_bonus[5].endgame, rook_threat_bonus[5].endgame, "rook_threat_bonus[5].endgame", true});
}

long double errors[10000];

void find_best_k(vector<Parameter> &parameters) {
    // Clear errors
    for (int i = 0; i < 10000; ++i) {
        errors[i] = -2.0L;
    }

    int min = 80, max = 120;

    k = (long double)(min) / 100.0L;
    long double min_err = find_error(parameters);
    errors[min] = min_err;
    cout << "errors[" << min << "]:\t" << min_err << endl;

    k = (long double)(max) / 100.0L;
    long double max_err = find_error(parameters);
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
                errors[max] = find_error(parameters);
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
                errors[min] = find_error(parameters);
            }
            min_err = errors[min];
            cout << "errors[" << min << "]:\t" << min_err << endl;
        }
    }
}

void tune() {
    cout.precision(32);
    vector<Parameter> initial_params;
    init_parameters(initial_params);
    find_best_k(initial_params);
    cout << "best k: " << k << endl;

    double best_error = find_error(initial_params);
    cout << "initial error:\t" << best_error << endl;
    vector<Parameter> best_guess = initial_params;
    bool improving = true;

    while (improving) {
        improving = false;
        for (unsigned pi = 0; pi < initial_params.size(); pi++) {
            vector<Parameter> new_guess = best_guess;
            new_guess[pi].value += best_guess[pi].increasing ? 1 : -1;
            if (new_guess[pi].value < 0) {
                continue;
            }
            set_parameter(&new_guess[pi]);
            double new_error = find_error(new_guess);

            if (new_error < best_error) {
                best_error = new_error;
                best_guess = new_guess;
                best_guess[pi].increasing = true;
                improving = true;
                cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << " (best)" << endl;
            } else {
                cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << endl;
                new_guess[pi].value -= best_guess[pi].increasing ? 2 : -2;
                if (new_guess[pi].value < 0) {
                    continue;
                }
                set_parameter(&new_guess[pi]);
                new_error = find_error(new_guess);
                if (new_error < best_error) {
                    best_error = new_error;
                    best_guess = new_guess;
                    best_guess[pi].increasing = false;
                    improving = true;
                    cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << " (best)" << endl;
                } else {
                    cout << new_guess[pi].name << "[" << new_guess[pi].value << "]:\t" << new_error << endl;
                }
            }
        }
    }

    for (unsigned i = 0; i < best_guess.size(); ++i) {
        Parameter *param = &best_guess[i];
        cout << "best " << param->name << ": " << param->value << endl;
    }
}

#endif
