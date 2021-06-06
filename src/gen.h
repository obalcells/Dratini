#pragma once

#include "defs.h"
#include "board.h"

uint64_t get_attackers(int, bool, const Board*);
void get_attackers(int sq, bool attacker_side, const Board* board, uint64_t& bb);
void generate_moves(std::vector<Move>&, const Board*, bool quiesce = false);
void generate_evasions(std::vector<Move>&, const Board*);
void generate_captures(std::vector<Move>&, const Board*);
void generate_quiet(std::vector<Move>&, const Board*);

Move* new_generate_captures(Move* moves, const Board*);
Move* new_new_generate_captures(Move* moves, const Board*, Move* move_p);
Move* new_generate_quiet(Move* moves, const Board*);
