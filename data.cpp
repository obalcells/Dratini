#include <vector>
#include "defs.h"

char color[64];
char piece[64];
bool side;
bool xside;
char castling;
char enpassant;

bool is_testing = false;
float max_search_time = 5000.0; // ms // ms
int max_depth = 5;

std::vector<Move> move_stack;
std::vector<Move> taken_moves;
std::vector<std::pair<int,Move> > unordered_move_stack;

std::vector<std::vector<short int> > book;
bool book_deactivated;

long long random_value[9][64];
int n_entries = 1 << 20;
PV_Entry pv_table[1 << 20];
int history[2][64][64];

bool slide[6] = { false, false, true, true, true, false };

// 0 -> WHITE
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

// cw first-range movements available
int offset[6][8] = {
	{  0,  0,  0,  0,  0,   0,   0,   0 },
	{  6, 15, 17, 10, -6, -15, -17, -10 },
	{  7,  9, -7, -9,  0,   0,   0,   0 },
	{ -1,  8,  1, -8,  0,   0,   0,   0 },
	{ -1,  7,  8,  9,  1,  -7,  -8,  -9 },
	{ -1,  7,  8,  9,  1,  -7,  -8,  -9 },
};