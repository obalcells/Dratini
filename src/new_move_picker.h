#include <vector>
#include "defs.h"
#include "board.h"
#include "search.h"

class NewMovePicker {
public:
    NewMovePicker(Thread&, Move, bool quiesce = false);
    Move next_move();
    Move select_best();
    bool bad_capture(Move) const;
    void score_captures();
    void score_quiet(Move*);

    Thread *thread;
    Board* board;
    Move tt_move, killer_1, killer_2;
    Move move, moves[256], bad_captures[256];
    Move *move_p, *move_p_aux, *moves_end, *bad_captures_end;
    int aux, phase, scores[256];
    int *score_p_aux, *scores_end;
    bool quiesce;
};