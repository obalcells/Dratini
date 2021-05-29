#pragma once

#include "defs.h"
#include "board.h"

uint64_t get_attackers(int, bool, const Board*);
void generate_moves(std::vector<Move>&, const Board*, bool quiesce = false);
void generate_evasions(std::vector<Move>&, const Board*);
void generate_captures(std::vector<Move>&, const Board*);
void generate_quiet(std::vector<Move>&, const Board*);

int* new_generate_captures(int* moves, const Board*);
int* new_generate_quiet(int* moves, const Board*);
