#include <stdlib.h>
#include "defs.h"
#include "protos.h"
#include "data.h"

long long random_long() {
    int ans = 0;
    for(int i = 0; i < 64; i++) {
        ans ^= rand() << i;
    }
    // return rand() << 32 | rand();
    return ans;
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
    for(int entry_idx = 0; entry_idx < n_entries; entry_idx++) {
        pv_table[entry_idx] = PV_Entry();
    }
}

long long get_hash() {
    long long hash = 0ll;
    for(int pos = 0; pos < 64; pos++) {
        if(piece[pos] != EMPTY) {
            hash ^= random_value[piece[pos]][pos];
        }
    }
    // enpassant and castling idx
    hash ^= random_value[7][0];
    hash ^= random_value[8][0];
    hash ^= side;
    return hash;
}