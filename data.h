#pragma once
#include <vector>
#include "defs.h"

/* state of the game */
extern char color[64];
extern char piece[64];
extern char side;
extern char xside;
extern char castling;
extern char enpassant;

extern Move next_move;
extern std::vector<Move> move_stack;
extern std::vector<Move> taken_moves;
extern std::vector<std::pair<int,Move> > unordered_move_stack;

extern long long random_value[9][64];
extern int n_entries;
extern PV_Entry pv_table[1 << 20];
extern int history[2][64][64];

extern int MAX_DEPTH;
extern bool TESTING;
extern float MAX_SEARCH_TIME;

/* helper constants */
extern int piece_value[6];
extern bool slide[6];
extern int initial_color[64];
extern int initial_piece[64];
extern int offset[6][8];
