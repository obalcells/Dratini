#pragma once

#include <vector>
#include "defs.h"

/* global vars */
extern std::vector<Move> move_stack;
extern std::vector<Move> taken_moves;
extern std::vector<std::pair<int,Move> > unordered_move_stack;
extern long long random_value[9][64];
extern int history[2][64][64];
extern std::vector<std::vector<Move> > book;
extern bool book_deactivated;

/* helper constants */
extern int piece_value[6];
extern bool slide[6];
extern int initial_piece[64];
extern int initial_color[64];
extern int offset[6][8];