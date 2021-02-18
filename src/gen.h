#pragma once

#include "defs.h"
#include "board.h"

void generate_moves(std::vector<Move>&, Board*, bool quiesce = false);
void generate_evasions(std::vector<Move>&, const Board*);
void generate_captures(std::vector<Move>&, const Board*);
void generate_quiet(std::vector<Move>&, const Board*);
