
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
struct Move { int from;	int to; int captured; };
void init_board();
void print_board();
Move make_move(int, int, int);
void undo_move(Move);
bool is_move_valid(int, int);

/* main.cpp */
int main();
std::pair<int,int> parse_move(std::string);

