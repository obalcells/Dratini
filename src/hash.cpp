#include <random>
#include <cstdint>
#include <cinttypes>
#include <vector>
#include "board.h"
#include "data.h" 

/* We don't want to rng's (the other is in new_board.h).
   We will delete this in the future */
namespace {
    /* pseudo-random number generator */
    std::string str_seed = "Dratini is fast!";
    std::seed_seq seed(str_seed.begin(), str_seed.end());
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
}

long long random_long() {
    return dist(rng);
}

void init_zobrist() {
    for(int piece = PAWN; piece <= KING; piece++) {
        for(int pos = 0; pos < 64; pos++) {
            random_value[piece][pos] = random_long();
        }
    }
    // enpassant and castling index
    random_value[7][0] = random_long();
    random_value[8][0] = random_long();
    for(int from_sq = 0; from_sq < 64; from_sq++) {
        for(int to_sq = 0; to_sq < 64; to_sq++) {
            history[0][from_sq][to_sq] = 0;
            history[1][from_sq][to_sq] = 0;
        }
    }
}

long long get_hash(const Position& position) {
    long long hash = 0ll;
    for(int pos = 0; pos < 64; pos++) {
        if(position.piece[pos] != EMPTY) {
            hash ^= random_value[position.piece[pos]][pos];
        }
    }
    // enpassant and castling idx
    hash ^= random_value[7][0];
    hash ^= random_value[8][0];
    hash ^= position.side;
    return hash;
}