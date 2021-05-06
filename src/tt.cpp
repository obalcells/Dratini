#include <cstdint>
#include <iostream>
#include <cinttypes>
#include <stdlib.h>
#include "tt.h"
#include "defs.h"

void TranspositionTable::allocate(int mb_size) {
    // we want the size of the table to be a power of two
    for(tt_size = 2; tt_size <= mb_size; tt_size *= 2);
    // it works too with -1 but we would need some extra checks in the loops
    tt_size = ((tt_size / 2) << 20) / sizeof(Entry);
    tt_mask = tt_size - 4;
    free(tt);
    tt = (Entry *) malloc(tt_size * sizeof(Entry));
    clear();
}

void TranspositionTable::clear() {
    Entry* entry;
    for(entry = tt; entry < tt + tt_size; entry++) {
        entry->key = 0;
        entry->depth = 0;
        entry->date = 0;
        entry->bound = 0;
        entry->move = NULL_MOVE;
        entry->score = 0;
    }
}

bool TranspositionTable::retrieve(uint64_t& key, Move& move, int& score, int& bound, int alpha, int beta, int depth, int ply) {
    Entry* entry;
    entry = tt + (key & tt_mask);

    for(int i = 0; i < 4; i++) {
        if(entry->key == key) {
            entry->date = tt_date;
            bound = entry->bound;
            move = entry->move;
            if(entry->depth >= depth) {
                score = entry->score;
                if(score <= -CHECKMATE) {
                    score += ply;
                } else if(score >= CHECKMATE) {
                    score -= ply;
                }
                if((entry->bound == EXACT_BOUND)
                || (entry->bound == UPPER_BOUND && score <= alpha)
                || (entry->bound == LOWER_BOUND && score >= beta))
                    return true;
            }
            break;
        }
        entry++;
    }
    return false;
}

// returns true if the position was found in the table
// bool TranspositionTable::retrieve_move(uint64_t& key, Move& move) {
//     Entry* entry;
//     entry = tt + (key & tt_mask);
//     for(int i = 0; i < 4; i++) {
//         if(entry->key == key) {
//             entry->date = tt_date;
//             move = entry->move;
//             return true;
//         }
//         entry++;
//     }
//     return false;
// } 

void TranspositionTable::save(uint64_t key, Move move, int score, int bound, int depth, int ply) {
    Entry *entry, *replace = NULL;
    entry = tt + (key & tt_mask);
    int oldest = -1, age;

    for(int i = 0; i < 4; i++) {
        if(entry->move == NULL_MOVE) {
            // cout << "Putting the thing in an empty slot" << endl;
            oldest = -2;
            replace = entry;
            break;
        } else if(bound == EXACT_BOUND) {
            oldest = -3;
            replace = entry;
            break;
        }
        // we determine which entry is more valuable
        age = ((tt_date - entry->date) & 255) * 256 + 255 - entry->depth;
        if(age > oldest) {
            replace = entry;
            oldest = age;
        } 
        entry++;
    }

    if(oldest != -1) {
        replace->key = key;
        replace->depth = depth;
        replace->date = tt_date;
        replace->bound = bound;
        replace->move = move;
        replace->score = score;
    }
}

int TranspositionTable::how_full() const {
    Entry* entry;
    int n_full = 0, n_checks = 1;
    for(entry = tt; entry < tt + tt_size; entry++) {
    // for(entry = tt; entry < tt + 2000000 && entry < tt + tt_size; entry++) {
        n_checks++;
        if(entry->move != NULL_MOVE)
            n_full++;
    }
    return n_full;
    // cout << "We did " << n_checks << " checks" << endl;
    // cout << "N of full is " << n_full << endl;
    // return (n_full * 100) / n_checks; // percentage of occupied
}
