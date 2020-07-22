#include <vector>
#include <iostream>
#include <cassert>
#include "defs.h"
#include "protos.h"
#include "data.h"
#include "board.h"

void think(int seconds) {
  for(int max_depth = 1; max_depth <= MAX_DEPTH; max_depth++) {
    search(max_depth, true);
  }
}

int search(int depth, bool root = false) {
  if(++nodes & 1023 == 0) {
    std::cout << "Number of nodes: " << nodes << ", Sz of stack is " << move_stack.size() << '\n';
  }

  if(depth <= 0) {
    return eval();
  }

  int first_move = (int)move_stack.size();
  generate_moves(); // will we reuse move generation? Not for now
  int last_move = (int)move_stack.size() - 1;

  Move best_move;
  int best_score = 99999;

  for(int i = last_move; i >= first_move; i--) {
    int from_before = (int)move_stack[i].from;
    int to_before = (int)move_stack[i].to;

    make_move((int)move_stack[i].from, (int)move_stack[i].to, QUEEN);
    //std::cout << from_before << " -> " << to_before << '\n'; 
    //print_board();

    int score = search(depth - 1);

    if(side == WHITE && (score == 99999 || score > best_score)) {
      best_move = move_stack[i];
      best_score = score;
    } else if(side == BLACK && (score == 99999 || score < best_score)) {
      best_move = move_stack[i];
      best_score = score;
    }

    assert(from_before == (int)move_stack[i].from);
    assert(to_before == (int)move_stack[i].to);

    take_back(move_stack[i]);
    move_stack.pop_back();
  }

  if(root) next_move = best_move;
  return best_score;
}

int eval() {
  /* to be done later */
  return 0;
}

