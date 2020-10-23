#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>
#include <sys/timeb.h>
#include "defs.h"
#include "protos.h"
#include "data.h"
#include "board.h"
#include "gen.h"
#include "move.h"
// #include "eval.h"
#include "eval_tscp.h"
#include "hash.h"

std::chrono::time_point<std::chrono::system_clock> initial_time;
int nodes, printed_points;
bool stop_search;

float ellapsed_time() {
  auto time_now = std::chrono::system_clock::now();
  std::chrono::duration<float, std::milli> duration = time_now - initial_time;
  return duration.count();
}

void think() {
  initial_time = std::chrono::system_clock::now();
  nodes = 0;
  printed_points = 0;
  int depth = 0;
  stop_search = false;
  Move best_move; 
  for(depth = 4; !stop_search; depth += 2) {
    nodes = 0;
    search(-999999, 999999, depth);
    if(!timeout()) best_move = next_move;
    if(stop_search == true) {
      depth -= 2; // max-depth where we did full-search
    }
  }
  std::cout << '\n'; // .....
  next_move = best_move; 
  if(!TESTING) {
    std::cout << "Searched a maximum depth of: " << depth << endl;
    std::cout << "Best move is: " << str_move(next_move.from, next_move.to) << endl;
    std::cout << "Nodes searched: " << nodes << endl;
    std::cout << "Ellapsed time: " << ellapsed_time() << " ms" << endl;
    std::cout << "Avg time per node: " << double(nodes) / ellapsed_time() << " ms" << endl;
    std::cout << "........................................" << endl << endl;
  }
  while(!move_stack.empty()) {
    move_stack.pop_back();
  }
}

int search(int alpha, int beta, int depth) {
  if(timeout()) {
    return alpha;
  }

  ++nodes;

  if(in_check(side)) {
    depth++;
  } else if(depth == 0) {
    return quiescence_search(alpha, beta);
  }

  // we check if search has already been performed for this state
  long long state_key = get_hash();
  int state_idx = state_key & (n_entries - 1);
  if(pv_table[state_idx].state_key == state_key && pv_table[state_idx].alpha >= alpha && pv_table[state_idx].alpha <= beta) {
    next_move = pv_table[state_idx].move; // what if there is a collision and we are at the root?    
    return pv_table[state_key].alpha;
  }

  int first_move = (int)move_stack.size();
  generate_moves(); // moves are already sorted
  int last_move = (int)move_stack.size() - 1;

  // no moves were generated
  if(first_move == last_move + 1) {
    if(in_check(side)) {
      return -999999;
    } else {
      return 0;
    }
  } else if(is_draw()) {
    return 0;
  }

  Move best_move;

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    make_move(move.from, move.to, QUEEN); // this could be sped up
    taken_moves.push_back(move);
    int score = -search(-beta, -alpha, depth - 1);
    take_back(move);
    taken_moves.pop_back();
    move_stack.pop_back();

    if(stop_search) {
      return alpha;
    }

    // move increases the alpha-cutoff
    if(score > alpha) {
      best_move = move;
      alpha = score;
      if(score >= beta) {
        history[side][move.from][move.to] += depth * depth;
        age_history();
        // the move caused a beta-cutoff so it must be good
        // but it won't be picked by parent
        while(i-- > first_move) move_stack.pop_back();
        next_move = best_move;
        return beta;
      }
    }
  }

  if(empty_move(best_move)) return alpha;
  history[side][best_move.from][best_move.to] += depth * depth;
  age_history();
  pv_table[state_idx] = PV_Entry(state_key, alpha, best_move);
  next_move = best_move;

  return alpha;
}

int quiescence_search(int alpha, int beta) {
  int score = eval_tscp();
  if(score >= beta) {
    return beta;
  } else if(score > alpha) {
    alpha = score;
  }

  int first_move = (int)move_stack.size();
  generate_capture_moves();
  int last_move = (int)move_stack.size() - 1;

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    make_move(move.from, move.to, QUEEN);
    taken_moves.push_back(move);
    score = -quiescence_search(-beta, -alpha);
    take_back(move);
    taken_moves.pop_back();
    move_stack.pop_back();

    if(stop_search) {
      return alpha;
    }
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

void age_history() {
  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 64; j++) {
      for(int k = 0; k < 64; k++) {
        history[i][j][k] = history[i][j][k] >> 1; // faster division by two
      }
    }
  }
}

bool timeout() {
  float et = ellapsed_time(); 
  if(et >= MAX_SEARCH_TIME) {
    stop_search = true;
    return true;
  }
  float threshold_time = float(printed_points) * (MAX_SEARCH_TIME / 40.0);
  if(et >= threshold_time) {
    std::cout << ".";
    std::cout.flush();
    printed_points++;
  }  
  return false;
}