
int color[64];
int piece[64];
int side;
int flags; //stores castling as well as en-passant flags

int piece_value[6] = { 100, 300, 300, 500, 900 };
bool slide[6] = { false, false, true, true, true, false}; 

//inverted
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

//cw first-range movements available
int offset[6][8] = {
	{  0,  0,  0,  0,  0,   0,   0,   0 },
	{  6, 15, 17, 10, -6, -15, -17, -10 },
	{  7,  9, -7, -9,  0,   0,   0,   0 },
	{ -1,  8,  1, -8,  0,   0,   0,   0 },
	{ -1, -7,  8,  9,  1,  -7,  -8,  -9 },
	{ -1, -7,  8,  9,  1,  -7,  -8,  -9 },
};


