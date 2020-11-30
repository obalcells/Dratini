#pragma once

#include <cstdint>
#include <cinttypes>

using BitBoard = uint64_t;
using Move = uint16_t;

class UndoMove {
    
};

class Position {
    Position();
    ~Position();
    bool move_valid(Move);
    void make_move(Move);
    void make_move(Move);
    void take_back();
};