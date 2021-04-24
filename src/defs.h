#pragma once

#include <cstdint>
#include <cinttypes>
#include <vector>
#include <iostream>
#include <cstdint>
#include <tmmintrin.h>

#define ll long long
#define endl '\n'

using std::cout;
using std::cin;
using std::cerr;

#define RESET_COLOR "\033[0m"
#define EMPTY_COLOR "\033[37m"
#define BLACK_COLOR "\033[36m"
#define WHITE_COLOR "\033[37m"
#define GREEN_COLOR "\x1B[32m"
#define BLUE_COLOR "\x1B[34m"
#define MAGENTA_COLOR "\x1B[35m"
#define RED_COLOR "\x1B[31m"

#define NO_ENPASSANT 8

#define row(x)(x >> 3)
#define col(x)(x & 7)

typedef uint16_t Move;
#define Move(from, to, flag) (from | (to << 6) | (flag << 12))
#define get_from(move) (move & 63)
#define get_to(move) ((move >> 6) & 63)
#define get_flag(move) (move >> 12)
#define is_null(move) (move == NULL_MOVE)

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

const char piece_char[13] = { 
    'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k', '_' 
};

enum Sides {
	WHITE = 0,
	BLACK = 1
};

enum Pieces {
	PAWN = 0,
	KNIGHT = 1,
	BISHOP = 2,
	ROOK = 3,
	QUEEN = 4,
	KING = 5
};

enum SidePieces {
	WHITE_PAWN = 0,
	WHITE_KNIGHT = 1,
	WHITE_BISHOP = 2,
	WHITE_ROOK = 3,
	WHITE_QUEEN = 4,
	WHITE_KING = 5,
	BLACK_PAWN = 6,
	BLACK_KNIGHT = 7,
	BLACK_BISHOP = 8,
	BLACK_ROOK = 9,
	BLACK_QUEEN = 10,
	BLACK_KING = 11,
    EMPTY = 12
};

enum MoveTypes {
	NULL_MOVE,
	QUIET_MOVE,
    CAPTURE_MOVE, 
	ENPASSANT_MOVE,
	CASTLING_MOVE,
	KNIGHT_PROMOTION,
	BISHOP_PROMOTION,
	ROOK_PROMOTION,
	QUEEN_PROMOTION
};

enum Castling {
    WHITE_QUEEN_SIDE,
    WHITE_KING_SIDE,
    BLACK_QUEEN_SIDE,
    BLACK_KING_SIDE
};

const int piece_value[12] = {
	100, 350, 350, 500, 1000, 0,
	100, 350, 350, 500, 1000, 0 
};

const int CHECKMATE = 100000;
const int MIN_DEPTH_FOR_WINDOW = 5;
const int INITIAL_WINDOW_SIZE = 30;
const int MIN_NULL_MOVE_PRUNING_DEPTH = 2;
const int MAX_PLY = 32;
const int MIN_BETA_PRUNING_DEPTH = 8;
const int BETA_MARGIN = 85;
const int MAX_HISTORY_BONUS = 300;

const int HISTORY_MULTIPLIER = 32;
const int HISTORY_DIVISOR = 32;

struct Thread;
class Board;
typedef std::vector<Move> PV;

inline std::string get_str(Move move) {
    std::string ans = "";
    ans += char('a' + col(get_from(move)));
    ans += char('1' + row(get_from(move)));
    ans += char('a' + col(get_to(move)));
    ans += char('1' + row(get_to(move)));
    return ans;
}

inline uint64_t mask_sq(int sq) {
    return (uint64_t(1) << sq);
}

#ifndef _MSC_VER
#ifndef _WIN64
#ifndef __GNUG__
static const int bit_table[64] = {
    0,  1,  2,  7,  3, 13,  8, 19,
    4, 25, 14, 28,  9, 34, 20, 40,
    5, 17, 26, 38, 15, 46, 29, 48,
    10, 31, 35, 54, 21, 50, 41, 57,
    63,  6, 12, 18, 24, 27, 33, 39,
    16, 37, 45, 47, 30, 53, 49, 56,
    62, 11, 23, 32, 36, 44, 52, 55,
    61, 22, 43, 51, 60, 42, 59, 58
};
#endif
#endif
#endif

inline int lsb(uint64_t mask) {
#if defined(_MSC_VER) && defined(_WIN64)
    unsigned long index;
	_BitScanForward64(&index, mask);
	return index;
#elif defined(__GNUG__)
    return __builtin_ctzll(mask);
#else
    return bit_table[((mask & (~(mask) + 1)) * uint64_t(0x0218A392CD3D5DBF)) >> 58]
#endif
}

inline int pop_first_bit(uint64_t& mask) {
    int index = lsb(mask);
    mask &= mask - 1;
    return index;
}

const __m128i pop_count_mask = _mm_set1_epi8(15);

const __m128i pop_count_table = _mm_setr_epi8(
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
);

inline __m128i popcnt_8(__m128i mask) {
    const __m128i cnt_0 = _mm_shuffle_epi8(pop_count_table, _mm_and_si128(mask, pop_count_mask));
    const __m128i cnt_1 = _mm_shuffle_epi8(pop_count_table, _mm_and_si128(_mm_srli_epi16(mask, 4), pop_count_mask));
    return _mm_add_epi8(cnt_0, cnt_1);
}

inline __m128i popcnt_64(__m128i n) {
    const __m128i cnt_8 = popcnt_8(n);
    return _mm_sad_epu8(cnt_8, _mm_setzero_si128());
}

inline int popcnt(uint64_t mask) {
    const __m128i n = _mm_set_epi64x(00, mask);
    const __m128i cnt = popcnt_64(n);
    return _mm_cvtsi128_si32(cnt);
}

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

inline std::string pos_to_str(int sq) {
    std::string ans = "";
    ans += char('a' + col(sq));
    ans += char('1' + row(sq));
    return ans;
}

inline std::string move_to_str(const Move move) {
    if(move == NULL_MOVE) {
        return "0000";
    }
    std::string ans = "";
    ans += char('a' + col(get_from(move)));
    ans += char('1' + row(get_from(move)));
    ans += char('a' + col(get_to(move)));
    ans += char('1' + row(get_to(move)));
    if(get_flag(move) == QUEEN_PROMOTION) {
        ans += "=Q"; 
    } else if(get_flag(move) == BISHOP_PROMOTION) {
        ans += "=B";
    } else if(get_flag(move) == KNIGHT_PROMOTION) {
        ans += "=N";
    } else if(get_flag(move) == ROOK_PROMOTION) {
        ans += "=R";
    }
    return ans;
}

// inline std::string move_to_str(int from_sq, int to_sq) {
//     std::string ans = "";
//     ans += char('a' + col(from_sq));
//     ans += char('1' + row(from_sq));
//     ans += char('a' + col(to_sq));
//     ans += char('1' + row(to_sq));
//     // what if the pawn is promoting? let's just not use this function
//     return ans;
// }
