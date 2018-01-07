# Defenchess
### Overview

Defenchess is a free, open source UCI chess engine written in C++. This project has been created purely as a hobby and a challenge for ourselves to create a strong chess engine. It is meant to be used with a UCI compatible chess GUI.

### Features
- Magic bitboards
- Principal Variation and Quiescence search
- Razoring, futility and null move pruning
- Delta pruning
- Late move reductions
- Internal iterative deepening
- Transposition and pawn transposition table
- Move ordering with transposition table, killer, counter moves, static exchange evaluation and MVV/LVA
- Lazy SMP

### Future plans
- Singular extensions
- Pondering
- Better time management
- Opening book and tablebase support

### Special thanks
- Donna and the Chess Programming Wiki for the inspiration and helping us understand the basics of chess engines
- Stockfish for many of the ideas/optimizations used in our engine
- Many other open source chess engines for their ideas
