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

struct Move {
    char from;
    char to;
    char captured;
    char castling;
    char enpassant;
};

