/* state of the game */
extern int color[64];
extern int piece[64];
extern int side;
extern int xside;
extern int castling;
extern int enpassant;
extern std::vector<Move> move_stack;
extern Move next_move;
extern int MAX_DEPTH;
extern int nodes;

/* helper constants */
extern int piece_value[6];
extern bool slide[6];
extern int initial_color[64];
extern int initial_piece[64];
extern int offset[6][8];
