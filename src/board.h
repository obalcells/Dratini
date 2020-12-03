#pragma once

#include <string>
#include "defs.h"

class Position {
public:
    Position() { init_board(); }
    ~Position() {}
    /* moves */
    bool check_coloring();
    bool check_castling(Move);
    bool check_enpassant(Move);
    bool move_valid(Move);
    void make_move(Move);
    void take_back(Move);
    /* board */
    void init_board();
    void print_board();
    bool in_check(bool defender_side);
    bool is_draw();
    int game_over();
    bool same(const Position& other_position);
    /* we have to mark these as publics to not have to modify tscp */
    char color[64];
    char piece[64];
    char side;
    char xside;
    char castling;
    char enpassant;
    int move_cnt;
private:
    bool is_attacked(int pos, bool attacker_side);
};