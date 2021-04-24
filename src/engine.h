#pragma once

#include "board.h"
#include "tt.h"

struct Engine {
    Board board;
    int max_depth = 32;
    int nodes, score;
    float search_time;
    bool stop_search, is_searching;
    // bool is_pondering;
    Move best_move, ponder_move;
    int max_search_time = 8000;

    Engine() {
        board = Board();
        tt.allocate(128);
    }

    Engine(const std::string& fen) {
        board = Board(fen);
        tt.allocate(128);
    }

    void set_position() {
        board = Board();
        // tt.clear();
    }

    void set_position(const std::string& fen) {
        board = Board(fen);
        // tt.clear();
    }

    // void search() {
    //     stop_search = false;
    //     best_move = NULL_MOVE, ponder_move = NULL_MOVE;
    //     think(*this, best_move, ponder_move);
    //     assert(stop_search == true);
    // }
};
