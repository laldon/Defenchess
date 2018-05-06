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

#include "uci.h"
#include "search.h"
#include <sys/time.h>
#include "test.h"
#include <fstream>
#include "eval.h"
#include "timecontrol.h"
#include "bitbase.h"
#include "tt.h"
#include <vector>
#include <map>
#include <iostream>
#include "tb.h"

using namespace std;

vector<string> word_list;
Position *root_position;

vector<string> split_words(string s) {
    vector <string> tmp;
    unsigned l_index = 0;
    for (unsigned i = 0 ; i < s.length() ; i++) {
        if (s[i] == ' ') {
            tmp.push_back(s.substr(l_index, i - l_index));
            l_index = i + 1;
        }
        if (i == s.length() - 1) {
            tmp.push_back(s.substr(l_index));
        }
    }

    return tmp;
}

Move uci2move(Position *p, string s) {
    Square from = (Square) (n2i(s[0]) + 8 * (s[1] - '0') - 8);
    Square to = (Square) (n2i(s[2]) + 8 * (s[3] - '0') - 8);
    Piece piece = p->pieces[from];
    uint8_t type_o = NORMAL;
    if (is_pawn(piece)) {
        if (row(to) == 0 || row(to) == 7) {
            type_o = PROMOTION;
        } else if (to == p->enpassant) {
            type_o = ENPASSANT;
        }
    } else if (is_king(piece) && std::abs(from - to) == 2) {
        type_o = CASTLING;
    }
    return _movecast(from, to, type_o);
}

bool word_equal(int index, string comparison_str) {
    if (word_list.size() > (unsigned) index)
        return word_list[index] == comparison_str;
    return false;
}

void uci() {
    cout << "id name Defenchess 1.2 x64" << endl << "id author Can Cetin & Dogac Eldenk" << endl;
#ifndef NDEBUG
    cout << "debug mode on" << std::endl;
#endif
    cout << "option name Hash type spin default 256 min 1 max 16384" << endl;
    cout << "option name Threads type spin default 1 min 1 max " << MAX_THREADS << endl;
    cout << "option name SyzygyPath type string default <empty>" << endl;
    cout << "uciok" << endl;
}

void perft() {
    if (word_equal(1, "test"))
        TEST_H::perft_test();
    else {
        uint64_t nodes = Perft(stoi(word_list[1]), root_position, true, is_checked(root_position));
        std::cout << "Nodes searched :  " << nodes << std::endl;
    }
}

void debug() {
    cout << bitstring(root_position->board);
    show_position_png(root_position);
    MoveGen movegen = new_movegen(root_position, 0, 0, 0, NORMAL_SEARCH, is_checked(root_position));
    while (Move move = next_move(&movegen) != no_move) {
        cout << move_to_str(move) << " ";
    }
    cout << endl;
}

void quit() {
    is_timeout = true;
    exit(EXIT_SUCCESS);
}

void stop() {
    is_timeout = true;
}

void generate() {
    generate_bitbase();
}

void isready() {
    cout << "readyok" << endl;
}

void eval() {
    print_eval(root_position);
}

void go() {
    is_timeout = false;
    int black_remaining = 0;
    int white_remaining = 0;
    int black_increment = 0;
    int white_increment = 0;
    moves_to_go = 0;
    think_depth_limit = MAX_PLY;

    if (word_equal(1, "movetime")) {
        moves_to_go = 1;
        myremain = stoi(word_list[2]) * 99 / 100;
        total_remaining = myremain;
    }
    else if (word_equal(1, "infinite")) {
        moves_to_go = 1;
        myremain = 3600000;
    }
    else if (word_equal(1, "depth")) {
        moves_to_go = 1;
        myremain = 3600000;
        think_depth_limit = stoi(word_list[2]);
    }
    else if (word_list.size() > 1) {
        for (unsigned i = 1 ; i < word_list.size() ; i += 2) {
            if (word_list[i] == "wtime")
                white_remaining = stoi(word_list[i + 1]);
            if (word_list[i] == "btime")
                black_remaining = stoi(word_list[i + 1]);
            if (word_list[i] == "winc")
                white_increment = stoi(word_list[i + 1]);
            if (word_list[i] == "binc")
                black_increment = stoi(word_list[i + 1]);
            if (word_list[i] == "movestogo")
                moves_to_go = stoi(word_list[i + 1]);
            if (word_list[i] == "infinite")
                myremain = 3600000;
            if (word_list[i] == "depth")
                think_depth_limit = stoi(word_list[i + 1]);
        }

        TTime t = get_myremain(
            root_position->color == white ? white_increment : black_increment,
            root_position->color == white ? white_remaining : black_remaining,
            moves_to_go
        );
        myremain = t.optimum_time;
        total_remaining = t.maximum_time;
    }

    std::thread think_thread (think, root_position);
    think_thread.detach();
}

void startpos() {
    root_position = start_pos();

    if (word_equal(2, "moves")) {
        for (unsigned i = 3 ; i < word_list.size() ; i++) {
            Move m = no_move;
            if (word_list[i].length() == 4) {
                m = uci2move(root_position, word_list[i]);
            } else if (word_list[i].length() == 5) {
                if (word_list[i][4] == 'n') {
                    m = _promoten(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'r') {
                    m = _promoter(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'b') {
                    m = _promoteb(uci2move(root_position, word_list[i]));
                } else {
                    m = _promoteq(uci2move(root_position, word_list[i]));
                }
            }
            if (m && is_pseudolegal(root_position, m)) {
                root_position = make_move(root_position, m);
            }
        }
    }
}

void cmd_fen() {
    string fen_str = word_list[2] + " " + word_list[3] + " " + word_list[4] + " " + word_list[5] + " " + word_list[6] + " " + word_list[7];

    root_position = import_fen(fen_str.c_str());

    if (word_equal(8, "moves")) {
        for (unsigned i = 9 ; i < word_list.size() ; i++) {
            Move m = no_move;
            if (word_list[i].length() == 4) {
                m = uci2move(root_position, word_list[i]);
            } else if (word_list[i].length() == 5) {
                if (word_list[i][4] == 'n') {
                    m = _promoten(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'r') {
                    m = _promoter(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'b') {
                    m = _promoteb(uci2move(root_position, word_list[i]));
                } else {
                    m = _promoteq(uci2move(root_position, word_list[i]));
                }
            }
            if (m && is_pseudolegal(root_position, m)) {
                root_position = make_move(root_position, m);
            }
        }
    }
}

void see() {
    Move move = uci2move(root_position, word_list[1]);
    cout << see_capture(root_position, move) << endl;
}

void cmd_position() {
    if (word_list[1] == "fen") 
        cmd_fen();
    if (word_list[1] == "startpos")
        startpos();
    get_ready();
}

void setoption() {
    if (word_list[1] != "name" || word_list[3] != "value") {
        return;
    }
    string name = word_list[2];
    string value = word_list[4];

    if (name == "Hash") {
        reset_tt(stoi(value));
    } else if (name == "Threads") {
        num_threads = std::min(MAX_THREADS, stoi(value));
    } else if (name == "SyzygyPath") {
        init_syzygy(value);
    }
}

void ucinewgame() {
    clear_tt();
}

void run_command(string s) {
    if (s == "ucinewgame")
        ucinewgame();
    if (s == "position")
        cmd_position();
    if (s == "go")
        go();
    if (s == "setoption")
        setoption();
    if (s == "eval")
        eval();
    if (s == "isready")
        isready();
    if (s == "uci")
        uci();
    if (s == "perft")
        perft();
    if (s == "debug")
        debug();
    if (s == "quit" || s == "exit")
        quit();
    if (s == "generate")
        generate();
    if (s == "stop")
        stop();
    if (s == "see")
        see();
}

void loop() {
    string in_str;
    init();
    root_position = start_pos();

    while (true) {
        getline(cin, in_str);
        word_list = split_words(in_str);
        if (word_list.size() > 0) {
            run_command(word_list[0]);
        }
    }

}

