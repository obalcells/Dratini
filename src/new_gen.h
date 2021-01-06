#pragma once

#include "defs.h"
#include "new_board.h"

extern void generate_moves(std::vector<NewMove>&, const BitBoard&, bool quiesce = false);
