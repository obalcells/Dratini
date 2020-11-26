#include <cstdint>
#include <cinttypes>
#include <stdlib.h>
#include "tt.h"

namespace {
    const int bound_mask = 3;
    const int date_mask  = ((1 << 8) - 1) ^ 3;  
}

void TranspositionTable::allocate(int mb_size) {
    // we want the size of the table to be a power of two
    for(tt_size = 2; tt_size <= mb_size; tt_size *= 2);
    // it works too with -1 but we would need some extra checks in the loops
    tt_mask = tt_size - 4;
    tt_size = ((tt_size / 2) << 20) / sizeof(Entry);
    free(tt);
    tt = (Entry *) malloc(tt_size * sizeof(Entry));
    clear();
}

void TranspositionTable::clear() {
    Entry* entry;
    for(entry = tt; entry < tt + tt_size; entry++) {
        entry->key = 0;
        entry->depth = 0;
        entry->flags = 0;
        entry->move = 0;
        entry->score = 0;
    }
}

// returns true/false if the move will fail high or low
bool TranspositionTable::retrieve_data(uint64_t key, int& move, int& score, int& flags, int alpha, int beta, int depth, int ply) {
    Entry* entry;
    entry = tt + (key & tt_mask);
    for(int cnt = 0; cnt < 4; cnt++) {
        if(entry->key == key) {
            entry->flags = (entry->flags & EXACT_BOUND) | (tt_date << 2);
            move = entry->move;
            if(entry->depth >= depth) {
                flags = entry->flags;
                score = entry->score;
                /*
                if score is minimum we should add the ply
                if score is maximum we should subtract the ply 
                */
                if(((entry->flags & UPPER_BOUND) && score <= alpha)
                || ((entry->flags & LOWER_BOUND) && score >= beta))
                    return true;
            }
        }
    }
    return false;
}

// returns true if the position was found in the table
bool TranspositionTable::retrieve_move(uint64_t key, int& move) {
    Entry* entry;
    entry = tt + (key & tt_mask);
    for(int i = 0; i < 4; i++) {
        if(entry->key == key) {
            entry->flags = (entry->flags & EXACT_BOUND) | (tt_date << 2);
            move = entry->move;
            return true;
        }
    }
    return false;
} 

void TranspositionTable::save(uint64_t key, int move, int score, int bound, int depth, int ply) {
    Entry *entry, *replace = NULL;
    entry = tt + (key & tt_mask);
    int oldest = -1, age;
    for(int i = 0; i < 4; i++) {
        if(entry->key == key) {
            if(!entry->move || bound == EXACT_BOUND) {
                replace = entry;
            }
            break;
        }
        // we determine which entry is more valuable
        age = entry->depth + tt_date - get_date(entry->flags) + 256;
        if(tt_date < get_date(entry->flags))
            age += 256;
        if(age > oldest) {
            replace = entry;
            oldest = age;
        } 
    }
    if(oldest != -1) {
        replace->key = key;
        replace->depth = depth;
        replace->flags = bound | (tt_date << 2);
        replace->move = move;
        replace->score = score;
    }
}

float TranspositionTable::how_full() const {
    Entry* entry;
    int n_full = 0, n_checks = 1;
    for(entry = tt; entry < tt + 6000 && entry < tt + tt_size; entry++) {
        n_checks++;
        if(entry->move != 0)
            n_full++;
    }
    return float(n_full) / float(n_checks); // percentage of occupied
}
