#pragma once

#include "board.h"
#include "tt.h"

struct Engine {
    Board board;
    int max_depth;
    int nodes, score;
    int search_time;
    bool stop_search, is_searching, is_pondering;
    Move best_move, ponder_move;
    int max_search_time;

    Engine() {
        max_depth = 64;
        nodes = score = 0;
        search_time = 0.0;
        stop_search = is_searching = false;
        best_move = ponder_move = NULL_MOVE; 
        max_search_time = 30000;
        board = Board();
    }

    void set_position() {
        board = Board();
    }

    void set_position(const std::string& fen) {
        board = Board(fen);
    }

    void reset() {
        board = Board();
    }

    // void search() {
    //     stop_search = false;
    //     best_move = NULL_MOVE, ponder_move = NULL_MOVE;
    //     think(*this, best_move, ponder_move);
    //     assert(stop_search == true);
    // }
};

extern Engine engine;