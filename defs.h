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

#define endl '\n'

struct Move {
    int from;
    int to;
    int captured;
    int castling;
    int enpassant;
    bool promotion;

    Move() {
      from = -1; to = -1; captured = EMPTY;
      castling = 0; enpassant = 0; promotion = false; 
    }

    Move(int _from, int _to, int _captured, int _castling, int _enpassant) {
      from = _from; to = _to; captured = _captured;
      castling = _castling; enpassant = _enpassant; promotion = false;
    }

    Move(int _from, int _to, int _captured, int _castling, int _enpassant, bool _promotion) {
      from = _from; to = _to; captured = _captured;
      castling = _castling; enpassant = _enpassant; promotion = _promotion;
    }
};

