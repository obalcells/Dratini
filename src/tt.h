#pragma once

#include <cstdint>
#include <cinttypes>
#include "defs.h"

enum Bound {
  NONE,
  UPPER_BOUND,
  LOWER_BOUND,
  EXACT_BOUND
};

struct Entry {
    uint64_t key;
    uint8_t depth;  
    uint8_t date;
    uint8_t bound;
    int16_t move;
    int16_t score;
};

class TranspositionTable {  
public:
    void allocate(int mb_size);
    void clear();
    int size() const { return tt_size; }
    void age() { tt_date = (tt_date + 1) & 255; }
    int how_full() const;
    // pass reference of position object in the future
    bool retrieve(uint64_t& key, Move& move, int& score, int& bound, int alpha, int beta, int depth, int ply);
    // bool retrieve_move(int64_t& key, Move& move);
    void save(uint64_t key, Move move, int score, int bound, int depth, int ply);
private:
    // static_assert(sizeof(Entry) == 16, "Entry must have size 16");
    int tt_size, tt_mask, tt_date;
    Entry* tt; 
};

extern TranspositionTable tt;
