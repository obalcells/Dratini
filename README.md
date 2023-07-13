![Title](https://i.imgur.com/5wb21Si.png)

It does minimax search with alpha-beta pruning together with some other optimizations such as magic-move generation, null-move optimization and quiescence search. The board representation is based on bitboards. The engine isn't that good yet, although it's not easy to defeat it (:
It's ELO lies around 1800.

## Other engines and chess programming resources

1. [Chessprogramming Wiki](https://www.chessprogramming.org/Main_Page).
2. [Tom Kerrigan's Simple Chess Program Source Code](http://www.tckerrigan.com/Chess/TSCP/)
3. [Stockfish](https://github.com/official-stockfish/Stockfish)

## TO-DO List

- [X] Speed statistics
- [X] More efficient bug-free transposition table
- [X] Unit tests
- [X] Self-play script
- [X] New board move making and validity checking
- [X] Testing new board representation
- [X] Move generation
- [X] Testing move generation
- [X] Better Move ordering
- [X] Many more things I forgot to write...
- [X] ...
- [X] New Search optimizations
- [X] Getting Dratini's UCI protocol to work with xboard
- [X] Detailed analysis with pseudocode of other engines' search functions (Rodent III, Ethereal and Halogen)
- [X] Using cutechess to make Dratini vs. Dratini matches
- [X] Creating dir with docs and parallelized cutechess tournaments
- [X] Understand profiling and try to use it if it makes sense
- [X] Programming bench
- [X] Be able to play against Sungorus
- [X] Clone Sungorus eval function for benchmarking
- [X] Be able to benchmark in server
- [X] Test move picker
- [X] Speed up board class
- [X] Implement simpler (copied) search function
- [X] Implement move making and move validity checking benchmark
- [X] Implement perft and compare old versus new board functions versus Sungorus
- [X] Implement SEE Benchmark and improve SEE function speed
- [X] Implement changes in search function and test against previous version
- [X] Integrate NNUE code (only FEN probing) and test against previous version
- [ ] Finish AVX2 implementation and try to use AVX512
- [ ] Clean up unused code and macros

[What does Dratini mean?](https://www.pokemon.com/en/pokedex/dratini)
