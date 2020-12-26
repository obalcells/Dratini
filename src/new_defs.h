#pragma once

/*
#include <cstdint>
#include <cinttypes>

#define ll long long
#define endl '\n'

#define RESET_COLOR "\033[0m"
#define EMPTY_COLOR "\033[37m"
#define BLACK_COLOR "\033[36m"
#define WHITE_COLOR "\033[37m"
#define GREEN_COLOR "\x1B[32m"
#define RED_COLOR "\x1B[31m"

#define row(x)(x >> 3)
#define col(x)(x & 7)

#ifndef MAX_DEPTH
#define MAX_DEPTH 32
#endif

#ifndef MAX_SEARCH_TIME
#define MAX_SEARCH_TIME 5000.0
#endif

enum Sides {
	WHITE = 0,
	BLACK = 1,
	EMPTY = 6
};

enum Pieces {
	PAWN = 0,
	KNIGHT = 1,
	BISHOP = 2,
	ROOK = 3,
	QUEEN = 4,
	KING = 5
};

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

enum NewMoveTypes {
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

struct NewMove {
	uint16_t bits;
	NewMove() {
		bits = 0;
	}
    NewMove(int from, int to, int flag) {
        bits = from | (to >> 6) | (flag >> 12);
    }
    bool is_move_null() const {
        return bits == 0;
    }
    int get_from() const {
        return bits & 63;
    }
    int get_to() const {
        return (bits >> 6) & 63;
    }
    int get_flag() const {
        return (bits >> 12);
    }
};

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

inline int LSB(uint64_t mask) {
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

inline int LSB_pop(uint64_t &bb) {
    int index = LSB(bb);
    bb &= bb - 1;
    return index;
}
*/