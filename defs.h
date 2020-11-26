#pragma once
#include <vector>
#include <iostream>

#define ll long long
#define endl '\n'

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

#define ENPASSANT_INDEX 6
#define CASTLING_INDEX 7

#define row(x) (x >> 3)
#define col(x) (x & 7)

struct Move {
    char from;
    char to;
    char captured;
    char castling;
    char enpassant;
    bool promotion;
    Move() {
      from = 64; to = 64; captured = EMPTY;
      castling = 0; enpassant = 0; promotion = false;
    }
    Move(char _from, char _to) {
      from = _from; to = _to; captured = EMPTY;
      castling = 0; enpassant = 0; promotion = false;
    }
    Move(char _from, char _to, char _captured, char _castling, char _enpassant) {
      from = _from; to = _to; captured = _captured;
      castling = _castling; enpassant = _enpassant; promotion = false;
    }
    Move(char _from, char _to, char _captured, char _castling, char _enpassant, bool _promotion) {
      from = _from; to = _to; captured = _captured;
      castling = _castling; enpassant = _enpassant; promotion = _promotion;
    }
    bool operator <(const Move & b) const {
      return false;
    }
};

/* These helper functions will be removed from here very soon */
inline bool valid_pos(const int x) {
  return (x >= 0 && x < 64);
}

inline int abs(const int x) {
  return ((x >= 0) ? x : -x);
}

inline int distance(const int x, const int y) {
  return (abs(row(x) - row(y)) + abs(col(x) - col(y)));
}

inline bool valid_distance(const int x, const int y) {
  return valid_pos(x) && valid_pos(y) && (distance(x, y) <= 3);
}

inline std::string str_move(char from, char to) {
	std::string ans = "";
	ans += char('a' + col(from));
	ans += char('1' + row(from));
	ans += char('a' + col(to));
	ans += char('1' + row(to));
	return ans;
}

inline bool empty_move(Move m) {
	return m.from == 64 && m.to == 64;
}

inline bool parse_move(std::string raw_input, char & from, char & to) {
	if((int)raw_input.size() != 4) return false;
	int col_1 = raw_input[0] - 'a';
	int row_1 = raw_input[1] - '1';
	int col_2 = raw_input[2] - 'a';
	int row_2 = raw_input[3] - '1';
	if(row_1 >= 0 && row_1 < 8
	&& col_1 >= 0 && col_1 < 8
	&& row_2 >= 0 && row_2 < 8
	&& col_2 >= 0 && col_2 < 8) {
		from = row_1 * 8 + col_1; to = row_2 * 8 + col_2;
		return true;
	}
	return false;
}
