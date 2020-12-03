#pragma once

#include <cstdint>
#include <cinttypes>
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
#define GREEN_COLOR "\x1B[32m"
#define RED_COLOR "\x1B[31m"

#define NO_ENPASSANT 8
#define ENPASSANT_INDEX 6
#define CASTLING_INDEX 7

#define row(x)(x >> 3)
#define col(x)(x & 7)

#ifndef MAX_DEPTH
#define MAX_DEPTH 32
#endif

#ifndef MAX_SEARCH_TIME
#define MAX_SEARCH_TIME 5000.0
#endif

typedef uint16_t Move;

enum Squares {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

enum {
    NULL_MOVE = 0,
    NORMAL_MOVE = 0,
    ENPASSANT_MOVE = 1,
    CASTLING_MOVE = 2,
    PROMOTION_MOVE = 3
};

#define Move(from, to) (from | (to << 6))
#define from(move) (move & 63)
#define to(move) ((move >> 6) & 63)
#define is_null(move) (move == NULL_MOVE)

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

inline std::string move_to_str(Move move) {
    std::string ans = "";
    ans += char('a' + col(from(move)));
    ans += char('1' + row(from(move)));
    ans += char('a' + col(to(move)));
    ans += char('1' + row(to(move)));
    return ans;
}

inline bool parse_move(std::string raw_input, Move& move) {
    if ((int) raw_input.size() != 4) return false;
    int col_1 = raw_input[0] - 'a';
    int row_1 = raw_input[1] - '1';
    int col_2 = raw_input[2] - 'a';
    int row_2 = raw_input[3] - '1';
    if (row_1 >= 0 && row_1 < 8 &&
        col_1 >= 0 && col_1 < 8 &&
        row_2 >= 0 && row_2 < 8 &&
        col_2 >= 0 && col_2 < 8) {
			move = Move(row_1 * 8 + col_1, row_2 * 8 + col_2);
			return true;
    }
	return false;
}