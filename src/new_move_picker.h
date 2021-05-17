#include <vector>
#include "defs.h"
#include "board.h"
#include "search.h"

class NewMovePicker {
public:
    NewMovePicker(Thread&, Move, bool quiesce = false);
    Move next_move();
    Move select_best();
    bool bad_capture(Move);
    void score_captures();
    void score_quiet(int);
    int see(Move) const;
    int next_lva(const uint64_t&, const bool) const;

    Thread *thread;
    Board* board;
    Move tt_move, killer_1, killer_2;
    std::vector<Move> moves, bad_captures;
    std::vector<int> scores;
    int next, badp, phase;
    bool quiesce;
};