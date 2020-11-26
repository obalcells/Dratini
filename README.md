# Just another C++ chess engine

This may not be the best chess engine, but it is a chess engine.

It does minimax search with alpha-beta pruning together with some other optimizations such as null-move optimization and quiescence search. It uses a classical 2-dimensional array for representing the board.

Dratini might be slow. However, it's still not easy to defeat it. I would guess that its ELO lies between 1300 and 1400. It can search around 500000 nodes in the main search function in about 5 seconds.

Useful chess programming resources:

1. [Chessprogramming Wiki](https://www.chessprogramming.org/Main_Page).
2. [Tom Kerrigan's Simple Chess Program Source Code](http://www.tckerrigan.com/Chess/TSCP/)
3. [Stockfish](https://github.com/official-stockfish/Stockfish)

## TO-DO List

- [X] Speed statistics
- [X] More efficient bug-free transposition table
- [ ] Unit tests
- [ ] Bitboards

## Why 'Dratini'?

I used to like Pokémon when I was young. Dratini is the name of a very weak Pokémon ([see picture](https://www.pokemon.com/en/pokedex/dratini)).