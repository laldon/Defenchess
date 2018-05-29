#!/usr/bin/python3

import chess.pgn
import time

pgn = open('games.pgn')
start = time.time()
fens = open('fens.txt', 'a')
num_games = 0
while True:
    game = chess.pgn.read_game(pgn)
    if not game:
        break
    result = game.headers['Result']
    node = game
    num_games += 1
    print('Extracting game {}({})'.format(num_games, time.time() - start))
    while not node.is_end():
        board = node.board()
        if board.fullmove_number <= 8 or not node.comment or board.is_check():
            node = node.variations[0]
            continue
        if 'M' in node.comment:
            break
        eval = float(node.comment.split('/')[0])
        if abs(eval) >= 6.0:
            break
        fens.write('{}|{}\n'.format(board.fen(), result))
        node = node.variations[0]
fens.close()
