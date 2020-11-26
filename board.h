#pragma once
#include <string>
#include "defs.h"

class Position {
public:
    Position() { init_board(); }
    ~Position() {}
    /* moves */
    int move_valid(char, char);
    void make_move(Move);
    Move make_move(char, char, char);
    void take_back(Move);
    /* board */
    void init_board();
    void print_board();
    bool in_check(bool defender_side);
    bool is_draw();
    int game_over();
    void save_snapshot(std::string snapshot_name);
    void load_snapshot(std::string snapshot_name);
    /* we have to mark these as publics to not have to modify tscp */
    char color[64];
    char piece[64];
    char side;
    char xside;
    char castling;
    char enpassant;
private:
    bool is_attacked(char pos, bool attacker_side);
};