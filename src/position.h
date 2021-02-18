#pragma once

#include <vector>

#include "board.h"
#include "defs.h"

class Position {
    public:
        Position();
        Position(const std::string& str, bool read_from_file = false);
        ~Position();

        Board& get_board();
        uint64_t get_key() const;
        int get_move_count() const;
        void print_board() const;
        bool in_check() const;
        bool only_kings_left() const;
        void debug(int) const;

        Move pair_to_move(int, int);
        void make_move(const Move);
        bool make_move_from_str(const std::string&);
        bool move_valid(const Move);
        bool move_valid(int, int);
        void take_back();
        void set_from_fen(const std::string&);

        std::vector<Board> board_history; 
        std::vector<Move> move_history;
};


