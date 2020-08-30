#include <vector>
#include <iostream>
#include <cassert>
#include "defs.h"
#include "protos.h"
#include "data.h"
#include "board.h"
// #include "eval.h"
#include "eval_tscp.h"
#include "hash.h"

#include <sys/timeb.h>

int get_ms() {
	struct timeb timebuffer;
	ftime(&timebuffer);
	return (timebuffer.time * 1000) + timebuffer.millitm;
}

void think(int seconds) {
  for(int initial_depth = MAX_DEPTH; initial_depth <= MAX_DEPTH; initial_depth += 2) {
    nodes = 0;
    int initial_time = get_ms();
    std::cout << "Searching with initial depth = " << initial_depth << "..." << endl;
    search(-999999, 999999, initial_depth);
    std::cout << "Best move is: " << next_move.from << " -> " << next_move.to << endl;
    std::cout << "Nodes searched: " << nodes << endl;
    std::cout << "Time elapsed: " << get_ms() - initial_time << " ms";
    std::cout << "........................................" << endl;
  }
}

int search(int alpha, int beta, int depth) {
  assert(alpha < beta);

  if(in_check(side)) {
    depth++;
  } else if(depth == 0) {
    // return eval_tscp() -> without quiescence search
    return quiescence_search(alpha, beta);
  }

  int first_move = (int)move_stack.size();
  generate_moves();
  int last_move = (int)move_stack.size() - 1;
  Move best_move;

  /* we check if search has already been performed for this state */
  ll state_key = get_hash();
  int state_idx = state_key % n_entries;

  Move pv_move = Move();
  if(history[state_idx].state_key == state_key) {
    pv_move = Move({ history[state_idx].from, history[state_idx].to });
    move_stack.push_back(pv_move); // first move to be visited
  }

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    if(move.from == pv_move.from && move.to == pv_move.to) continue;

    make_move(move.from, move.to, QUEEN); // this could be sped up
    int score = -search(-beta, -alpha, depth - 1);
    take_back(move);
    move_stack.pop_back();

    if(score > alpha) {
      best_move = move;
      alpha = score;

      if(score >= beta) {
        // the move caused a beta-cutoff so it must be good
        while(i-- > first_move) move_stack.pop_back();
        return beta;
      }
    }
  }

  if(history[state_idx].state_key == 0ll
    || (history[state_idx].state_key == state_key
       && history[state_idx].beta < beta)) {
    // hasn't been set before or our beta is higher
    history[state_idx] = PV_Entry({ get_hash(), beta, best_move.from, best_move.to });
  }

  next_move = best_move;
  return alpha;
}

int quiescence_search(int alpha, int beta) {
  int score = eval_tscp();

  if(score >= beta)
    return beta;
  else if(score > alpha)
    alpha = score;

  int first_move = (int)move_stack.size(); // before
  generate_capture_moves();
  int last_move = (int)move_stack.size() - 1; // after

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    make_move(move.from, move.to, QUEEN);
    score = -quiescence_search(-beta, -alpha);
    take_back(move);
    move_stack.pop_back();

    if(score > alpha) {
      alpha = score;
      if(beta <= alpha) {
        while(i-- > first_move) move_stack.pop_back();
        return beta;
      }
    }
  }

  return alpha;
}
