
#define col(x) (x & 7)
#define row(x) (x >> 3)

#define EMPTY 6

#define WHITE 0
#define BLACK 1

#define PAWN 0
#define KNIGHT 1
#define BISHOP 2
#define ROOK 3
#define QUEEN 4
#define KING 5

#define RESET_COLOR "\033[0m"
#define EMPTY_COLOR "\033[37m"
#define BLACK_COLOR "\033[36m"
#define WHITE_COLOR "\033[37m"

/* board.cpp */
// "info" var literally tells info about the move:
// (0) nothing
// (1) enpassant capture, (2) pawn promotion,
// (4) castling move, (8) first movement of king or rook
// more to be added...
struct Move { char from; char to; char captured; char info; };
void init_board();
void print_board();
Move make_move(int, int, int);
bool is_move_valid(int, int);
void takeback_move(Move);

/* helper functions */
int distance(int, int);
bool valid_pos(int);

/* main.cpp */
int main();

