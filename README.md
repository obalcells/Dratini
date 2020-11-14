# Just another C++ chess engine

This may not be the best chess engine, but it is a chess engine.

It does minimax search with alpha-beta pruning together with some other optimizations such as null-move optimization and quiescence search. It uses a classical 2-dimensional array for representing the board. I'm working on changing it to 0x88, since it is still too slow to be fun to play with.

Dratini might be slow. However, it's still not easy to defeat it. I would guess that its ELO lies between 1200 and 1300. It can search around 200000 nodes in the main search function in about 5 seconds.

If you kept reading until here, you might find the following sites useful:

1. [Chessprogramming Wiki](https://www.chessprogramming.org/Main_Page).
2. [Tom Kerrigan's Simple Chess Programm Source Code](http://www.tckerrigan.com/Chess/TSCP/)
3. [TSCP's unofficial guide](https://sites.google.com/site/tscpchess/home)

## TO-DO List

- [X] Speed statistics
- [ ] More efficient bug-free transposition table

## Why 'Dratini'?

I used to like Pokémon when I was young. Dratini is the name of a very weak Pokémon whose image you can find [here](https://www.pokemon.com/es/pokedex/dratini). The important thing about the name isn't only that Dratini is weak, but also that its evolution ([Dragonite](https://www.pokemon.com/es/pokedex/dragonite)) is one of the strongest Pokémons in the game. Metaphorically, this represents that Dratini is version I of the chess engine, and I'll eventually develop an improved version of the engine once I become better at programming.
