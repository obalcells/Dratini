#include <cstdint>
#include <cinttypes>

const int date_mask = (1 << 6) - 1;

enum Bound {
  NONE,
  UPPER_BOUND,
  LOWER_BOUND,
  BOUND_EXACT // = UPPER_BOUND | LOWER_BOUND
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
    void clear() { key = depth = flags = move = score = 0; }
};

class TranspositionTable {  
public:
    void allocate(int mb_size);
    void clear();
    void resize();
    float how_full();
    // pass reference of position object in the future
    bool retrieve_data(uint64_t key, int& move, int& score, int& flags, int alpha, int beta, int depth, int ply);
    bool retrieve_move(uint64_t key, int& move);
    void save(uint64_t key, int move, int score, int flags, int depth, int ply);
private:
    static_assert(sizeof(Entry) == 16, "Entry must have size 16");
    int tt_size, tt_mask, tt_date;
    Entry* tt; 
};