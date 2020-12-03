#pragma once

#include <vector>
#include "defs.h"

/* global variables */
std::vector<Move> move_stack;
std::vector<Move> taken_moves;
std::vector<std::pair<int, Move> > unordered_move_stack;
std::vector<std::vector<Move> > book;
long long random_value[9][64];
int history[2][64][64];
bool book_deactivated;

/* helper constants */
int piece_value[6] = { 100, 325, 325, 500, 1000, 10000 };

bool slide[6] = { false, false, true, true, true, false };

int initial_piece[64] = {
	3, 1, 2, 4, 5, 2, 1, 3,
	0, 0, 0, 0, 0, 0, 0, 0,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 2, 4, 5, 2, 1, 3
};

int initial_color[64] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
};

// cw first-range movements available
int offset[6][9] = {
	{  0,  0,  0,  0,  0,   0,   0,   0, 0 },
	{  6, 15, 17, 10, -6, -15, -17, -10, 0 },
	{  7,  9, -7, -9,  0,   0,   0,   0, 0 },
	{ -1,  8,  1, -8,  0,   0,   0,   0, 0 },
	{ -1,  7,  8,  9,  1,  -7,  -8,  -9, 0 },
	{ -1,  7,  8,  9,  1,  -7,  -8,  -9, 0 },
};