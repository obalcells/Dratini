#pragma once

#include <iostream>
#include "board.h"
#include "position.h"

void think(const Board&, bool*, Move&, Move&);
void aspiration_window(Thread&);
int search(Thread&, PV&, int, int, int);
int q_search(Thread&, PV&, int, int);
void update_quiet_history(Thread&, const Move, const std::vector<Move>&, const int);
void update_capture_history(Thread&, const Move, const std::vector<Move>&, const int);

struct Thread {
   int ply, index, depth, nodes, root_value;     
   Move best_move, ponder_move;
   Board board;
   std::vector<Move> move_stack;
   Move killers[MAX_PLY][2] = { NULL_MOVE };
   int quiet_history[2][64][64] = { 0 };
   int capture_history[6][64][6] = { 0 };
   bool* stop_search;

    Thread(Board _board, bool* _stop_search) {
        best_move = NULL_MOVE;
        root_value = -1;
        nodes = index = ply = 0;
        depth = 1;
        board = _board;
        stop_search = _stop_search;
        // *stop_search = false;
    }
};


