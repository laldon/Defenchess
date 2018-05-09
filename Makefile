# Defenchess, a chess engine
# Copyright 2017-2018 Can Cetin, Dogac Eldenk
# 
# Defenchess is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Defenchess is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Defenchess.  If not, see <http://www.gnu.org/licenses/>.

CC      = g++
CFLAGS  = -Wall -Wcast-qual -Wextra -Wshadow -pedantic -std=c++11 -m64
NAME    = Defenchess
OPT     = -O3
ext     =
ext2    = -pthread
version = 1.2

ifeq ($(OS),Windows_NT)
    ext = .exe
 	ext2 = 
endif

ifeq ($(OPTIMIZE),0)
    OPT = -O0
endif

all:
	$(CC) $(CFLAGS) $(OPT) src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_dev$(ext) $(ext2)
	./$(NAME)_dev$(ext)

winrelease:
	$(CC) $(CFLAGS) -static $(OPT) -DNDEBUG src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_$(version).exe $(ext2)

winfeature:
	$(CC) $(CFLAGS) -static $(OPT) -DNDEBUG src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_$(version)_$(feature).exe $(ext2)

release:
	$(CC) $(CFLAGS) $(OPT) -DNDEBUG src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_$(version)$(ext) $(ext2)

feature:
	$(CC) $(CFLAGS) $(OPT) -DNDEBUG src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_$(version)_$(feature)$(ext) $(ext2)

perft:
	$(CC) $(CFLAGS) $(OPT) -D__PERFT__ src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_perft$(ext) $(ext2)

debug:
	$(CC) $(CFLAGS) -g -D__DEBUG__ src/fathom/tbprobe.cpp src/*.cpp -o $(NAME)_debug$(ext) $(ext2)
