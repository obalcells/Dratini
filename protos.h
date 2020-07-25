
/* board.cpp */
void init_board();
void print_board();
Move make_move(int, int, int);
void undo_move(Move);
int move_valid(int, int);

/* main.cpp */
int main();

/* search.cpp */
void think(int);
int search(int, int, int);

/* eval.cpp */
int eval();
int eval_white_pawn(int, bool);
int eval_black_pawn(int, bool);
int eval_king(int, bool);
int eval_sorroundings(int, bool);

