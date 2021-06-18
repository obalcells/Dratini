#pragma once
#include "board.h"

void initialize_data();
int evaluate(const Board&);
int calculate_mat(const Board&);
int calculate_mat_p(Board* board);
int nnue_eval(Board& board);
