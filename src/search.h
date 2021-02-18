#pragma once

#include <iostream>
#include "board.h"
#include "position.h"

Move think(const Position& position);
void aspiration_window(Thread&);
int search(Thread&, PV&, int, int, int);
int q_search(Thread&, PV&, int, int);
void update_quiet_history(Thread&, const Move, const std::vector<Move>&, const int);
void update_capture_history(Thread&, const Move, const std::vector<Move>&, const int);

struct Thread {
   int ply, index, depth, nodes, root_value;     
   Move best_move, ponder_move;
   Position position;
   std::vector<Move> move_stack;
   Move killers[MAX_PLY][2] = { NULL_MOVE };
   int quiet_history[2][64][64] = { 0 };
   int capture_history[6][64][6] = { 0 };

    Thread(Position _position) {
        best_move = NULL_MOVE;
        nodes = index = ply = 0;
        depth = 1;
        position = _position;
    }
};


