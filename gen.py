#!/usr/bin/python3

import chess.pgn

pgn = open('games.pgn')
num_games = 1
while True:
    game = chess.pgn.read_game(pgn)
    if not game:
        break
    print('Extracting game {}'.format(num_games))
    node = game
    while not node.is_end():
        next_node = node.variations[0]
        if not node.comment or 'M' in node.comment:
            node = next_node
            continue
        fens = open('fens.txt', 'a')
        fens.write('{}|{}\n'.format(node.board().fen(), game.headers['Result']))
        node = next_node
    num_games += 1
    if num_games == 5000:
        break
