#pragma once

#include "defs.h"
#include "new_board.h"

extern void generate_moves(std::vector<NewMove>&, const BitBoard&, bool quiesce = false);

extern void generate_evasions(std::vector<NewMove>&, const BitBoard*);
extern void generate_captures(std::vector<NewMove>&, const BitBoard*);
extern void generate_quiet(std::vector<NewMove>&, const BitBoard*);
