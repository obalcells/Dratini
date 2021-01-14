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

        BitBoard& get_board();
        uint64_t get_key() const;
        int get_move_count() const;
        void print_board() const;
        bool in_check() const;
        bool only_kings_left() const;
        void debug(int) const;

        NewMove pair_to_move(int, int);
        void make_move(const NewMove&);
        bool make_move_from_str(const std::string&);
        bool move_valid(const NewMove&);
        bool move_valid(int, int);
        void take_back();
        void set_from_fen(const std::string&);

        std::vector<BitBoard> board_history; 
        std::vector<NewMove> move_history;
};


