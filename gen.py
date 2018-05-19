#!/usr/bin/python3

import chess.pgn
import time

pgn = open('games.pgn')
num_games = 1
start = time.time()
fens = open('fens.txt', 'a')
while True:
    print('Extracting game {}({})'.format(num_games, time.time() - start))
    game = chess.pgn.read_game(pgn)
    result = game.headers['Result']
    if not game:
        break
    node = game
    while not node.is_end():
        if not node.comment:
            node = node.variations[0]
            continue
        if 'M' in node.comment:
            break
        eval = float(node.comment.split('/')[0])
        if abs(eval) >= 6.0:
            break
        fens.write('{}|{}\n'.format(node.board().fen(), result))
        node = node.variations[0]
    num_games += 1
    if num_games == 5000:
        break
fens.close()
