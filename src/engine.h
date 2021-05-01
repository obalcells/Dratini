#pragma once

#include "board.h"
#include "tt.h"

struct Engine {
    Board board;
    int max_depth;
    int nodes, score;
    float search_time;
    bool stop_search, is_searching;
    // bool is_pondering;
    Move best_move, ponder_move;
    int max_search_time;

    Engine() {
        max_depth = 64;
        nodes = score = 0;
        search_time = 0.0;
        stop_search = is_searching = false;
        best_move = ponder_move = NULL_MOVE; 
        max_search_time = 8000;

        cerr << RED_COLOR << "Constructing board with empty fen" << RESET_COLOR << endl;
        board = Board();
        cerr << RED_COLOR << "Done constructing empty board arggg" << RESET_COLOR << endl;
        // tt.allocate(128);
        cerr << RED_COLOR << "Done constructing empty board arggg" << RESET_COLOR << endl;
    }

    void set_position() {
        board = Board();
        // tt.clear();
    }

    void set_position(const std::string& fen) {
        board = Board(fen);
        // tt.clear();
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