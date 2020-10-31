# Lightweight C++ Chess Engine

Probably the simplest chess engine you can find online. This engine has an ELO of around 1100 (?) and it's search function is a basic minimax with alpha-beta cutoffs and quiescential search (capture-moves only). The main search can do a depth of 7 half-moves in about 5 seconds.

If you are looking to better understand chess engines or even code your own, the following sites may be useful:
1. [Chessprogramming Wiki](https://www.chessprogramming.org/Main_Page).
2. [Tom Kerrigan's Simple Chess Programm Source Code](http://www.tckerrigan.com/Chess/TSCP/)
3. [TSCP's unofficial guide](https://sites.google.com/site/tscpchess/home)

# TO-DO List

## Short-term
- [x] Add checkmate detection
- [x] Test castling bug
- [x] Change eval to incentivise pawn openings
- [ ] Make it faster

## Long-term
- [ ] Code my own eval function
- [ ] Port to Javascript with Emscripten and setup github site
- [ ] Estimate bot's ELO
- [ ] Reach (non-quiescential) search depth of 8 or 9