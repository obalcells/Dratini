#pragma once

#include <vector>

#include "new_board.h"
#include "new_defs.h"
#include "defs.h"

class NewPosition {
    public:
        NewPosition();
        NewPosition(const std::string&);
        ~NewPosition();
        BitBoard get_board();
        uint64_t get_key();
        int get_move_count();
        void make_move(const NewMove&);
        bool make_move_from_str(const std::string&);
        bool move_valid(int, int);
        void take_back();
        void print_board();
        bool in_check();
        void set_from_fen(const std::string&);
        void debug();
    private:
        std::vector<BitBoard> board_history; 
        std::vector<NewMove> move_history;
};


