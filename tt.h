#include <cstdint>
#include <cinttypes>

const int bound_mask = 3;
const int date_mask  = ((1 << 8) - 1) ^ 3;  

#define get_bound(flags)  (flags & bound_mask)
#define get_date(flags)   (flags >> 2)

enum Bound {
  NONE,
  UPPER_BOUND,
  LOWER_BOUND,
  EXACT_BOUND
};

// Here's how the flags variable works:
// date    6 bits
// bound   2 bits   

struct Entry {
    uint64_t key;
    uint8_t depth;  
    uint8_t flags; 
    int16_t move;
    int16_t score;
};

class TranspositionTable {  
public:
    void allocate(int mb_size);
    void clear();
    int size() const { return tt_size; }
    float how_full() const;
    // pass reference of position object in the future
    bool retrieve_data(uint64_t key, int& move, int& score, int& flags, int alpha, int beta, int depth, int ply);
    bool retrieve_move(uint64_t key, int& move);
    void save(uint64_t key, int move, int score, int flags, int depth, int ply);
private:
    static_assert(sizeof(Entry) == 16, "Entry must have size 16");
    int tt_size, tt_mask, tt_date;
    Entry* tt; 
};