#include <stdlib.h>
#include "defs.h"
#include "protos.h"
#include "data.h"

long long random_value[8][64];
PV_Entry history[(int)4e5 + 7];
int n_entries = (int)4e5;

ll random_long() {
    int ans = 0;
    for(int i = 0; i < 64; i++)
        ans ^= rand() << i;
    return ans;
}

int random_int() {
    int ans = 0;
    for(int i = 0; i < 32; i++)
        ans ^= rand() << i;
    return ans;
}

void init_zobrist() {
    for(int piece = PAWN; piece <= KING; piece++)
        for(int pos = 0; pos < 64; pos++)
            random_value[piece][pos] = random_long();

    random_value[7][0] = random_long(); // enpassant index
    random_value[8][0] = random_long(); // castling index

    for(int entry_idx = 0; entry_idx < n_entries; entry_idx++) {
        history[entry_idx] = PV_Entry();
    }
}

ll get_hash() {
    ll hash = 0ll;
    for(int pos = 0; pos < 64; pos++)
        if(piece[pos] != EMPTY)
            hash ^= random_value[piece[pos]][pos];

    hash ^= random_value[7][0]; // enpassant index
    hash ^= random_value[8][0];
    hash ^= side;

    return hash;
}