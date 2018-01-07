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

#include "timecontrol.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

TTime moves_in_time(int increment, int remaining, int movestogo){
    int importance;
    importance = 5 * std::sqrt(movestogo);

    int average_time = remaining / movestogo;
    int extra = average_time * importance * 3 / 200;
    int spend = std::min(remaining - 100, average_time + extra + increment * (movestogo - 1) / movestogo);
    return {spend, remaining - 100};
}

TTime no_movestogo(int increment, int remaining) {
    if (remaining < increment * 2) {
        return {std::min(increment, remaining) - 100, std::min(increment, remaining) - 100};
    }

    int move_num = (root_ply + 1) / 2;
    int movestogo = std::max(20 + 3 * (50 - move_num) / 5 , 3);
    int average_time = remaining / movestogo;
    int extra = average_time * std::max(30 - move_num, 0) / 200;
    int spend = std::min(remaining - 100, average_time + extra + increment);
    return {spend, remaining - 100};
}

TTime get_myremain(int increment, int remaining, int movestogo){
    if (movestogo == 1) {
        // Certain GUIs are too garbage at timekeeping...
        return TTime{remaining - 100, remaining - 100};
    } else if (movestogo == 0) {
        return no_movestogo(increment, remaining);
    } else {
        return moves_in_time(increment, remaining, movestogo);
    }
}
