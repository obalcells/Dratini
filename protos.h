
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
int search(int, bool);
int eval();

