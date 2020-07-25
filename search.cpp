#include <vector>
#include <iostream>
#include <cassert>
#include "defs.h"
#include "protos.h"
#include "data.h"
#include "board.h"
#include "eval.h"

void think(int seconds) {
  for(int max_depth = 1; max_depth <= MAX_DEPTH; max_depth++) {
    search(-999999, 999999, max_depth);
  }
}

int search(int alpha, int beta, int depth) {
  if(++nodes & 1023 == 0) {
    std::cout << "Number of nodes: " << nodes << ", Sz of stack is " << move_stack.size() << '\n';
  }

  if(depth <= 0) {
    return eval();
  }

  int first_move = (int)move_stack.size();
  generate_moves();
  int last_move = (int)move_stack.size() - 1;
  Move best_move;

  for(int i = last_move; i >= first_move; i--) {
    make_move((int)move_stack[i].from, (int)move_stack[i].to, QUEEN);
    int score = search(alpha, beta, depth - 1);

    // remember your side is xside because sides just flipped
    if(side == BLACK && score > alpha) {
      best_move = move_stack[i];
      alpha = score;
    } else if(side == WHITE && score < beta) {
      best_move = move_stack[i];
      beta = score;
    }

    take_back(move_stack[i]);
    move_stack.pop_back();

    if(alpha >= beta) break;
  }

  next_move = best_move; // the root node will assign last
  if(side == WHITE) return alpha;
  else return beta;
}
