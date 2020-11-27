#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>
#include <sys/timeb.h>

#include "defs.h"
#include "data.h"
#include "board.h"
#include "book.h"
#include "gen.h"
#include "eval_tscp.h"
#include "hash.h"
#include "stats.h"
#include "tt.h"

void age_history();
int search(Position&, int, int, int);
int quiescence_search(Position&, int, int);
bool timeout();

int printed_points, depth;
Move move_root;
bool stop_search;

Move think(Position& position) {
  // get a move from the book if we can
  Move book_move = get_book_move();
  if(!empty_move(book_move)) { 
    std::cout << "Returning book move" << '\n';
    return book_move;
  } else {
    std::cout << "No book move available" << '\n';
  }
  /* reset statistics and vars */
  stats.init();
  stop_search = false;
  Move move_root_non_timeout = Move(); 
  move_root = Move();
  age_history();
  // we search iteratively with increasing depth until we run out of time
  for(depth = 4; !stop_search;) {
    search(position, -999999, 999999, depth);
    if(!stop_search) {
      move_root_non_timeout = move_root;
      depth += 2;
    } else if(stop_search) {
      depth -= 2; // max-depth where we did full-search
    }
  }
  std::cout << '\n'; // .....
  // in case that move_root was assigned after timeout
  move_root = move_root_non_timeout;
  if(!is_testing) {
    std::cout << "Searched a maximum depth of: " << depth << endl;
    std::cout << "Best move is: " << str_move(move_root.from, move_root.to) << endl;
    stats.display();
    std::cout <<
    "................................................................................"
    << endl << endl;
  }
  while(!move_stack.empty()) {
    move_stack.pop_back();
  }
  return move_root;
}

int search(Position& position, int alpha, int beta, int depth) {
  stats.change_phase(SEARCH);

  if(timeout()) {
    return alpha;
  }

  if(position.in_check(position.side)) {
    depth++;
  } else if(depth == 0) {
    int score = quiescence_search(position, alpha, beta);
    return score;
  }
  
  // we check if search has already been performed for this state
  long long state_key = get_hash(position);

  int retrieved_move = 0;

  if(tt.retrieve_move(state_key, retrieved_move)) {
    std::cout << "tt hit" << endl;
  }

  /*
  if(pv_table[state_idx].state_key == state_key && pv_table[state_idx].alpha >= alpha && pv_table[state_idx].alpha <= beta) {
    std::cout << "State already visited" << '\n';
    std::cout << pv_table[state_idx].state_key << '\n';
    std::cerr << "State_idx is " << state_idx << '\n';
    std::cerr << "Move that we have to make: " << str_move(pv_table[state_idx].move.from, pv_table[state_idx].move.to) << '\n';
    move_root.from = pv_table[state_idx].move.from; // what if there is a collision and we are at the root?    
    move_root.to = pv_table[state_idx].move.to; // what if there is a collision and we are at the root?    
    if(empty_move(move_root)) {
      std::cout << "State key is: " << state_key << endl;
      std::cout << str_move(move_root.from, move_root.to) << endl;
      assert(!empty_move(move_root));
    }
    std::cerr << "Before" << '\n';
    std::cerr << pv_table[state_key].alpha << '\n';
    std::cerr << "After" << '\n';
    return pv_table[state_key].alpha;
  }
  */

  int first_move = (int)move_stack.size();
  generate_moves(position); // moves are already sorted
  int last_move = (int)move_stack.size() - 1;

  // no moves were generated
  if(first_move == last_move + 1)
    return position.in_check(position.side) ? -999999 : 0;
  else if(position.is_draw())
    return 0;

  Move best_move;

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    position.make_move(move.from, move.to, QUEEN); // this could be sped up
    taken_moves.push_back(move);
    bool book_state_before = book_deactivated;
    int score = -search(position, -beta, -alpha, depth - 1);
    position.take_back(move);
    taken_moves.pop_back();
    move_stack.pop_back();
    book_deactivated = book_state_before;

    if(stop_search) {
      return alpha;
    }

    // move increases the alpha-cutoff
    if(score > alpha) {
      best_move = move;
      alpha = score;
      if(score >= beta) {
        history[position.side][move.from][move.to] += depth * depth;
        age_history();
        // the move caused a beta-cutoff so it must be good
        // but it won't be picked by parent
        while(i-- > first_move) move_stack.pop_back();
        /*
        if(state_key == -7145734984389955738) {
          std::cerr << "Storing move with alpha = " << alpha << '\n';
          std::cerr << "Move is " << (int)best_move.from << " " << (int)best_move.to << '\n';
        }
        pv_table[state_idx] = PV_Entry(state_key, alpha, best_move);
        if(state_key == -7145734984389955738) {
          std::cerr << "Retrieving " << pv_table[state_idx].alpha << '\n';
          std::cerr << "Retrieving " << pv_table[state_idx].alpha << '\n';
          std::cerr << "Retrieving " << pv_table[state_idx].alpha << '\n';
          std::cerr << "Retrieving " << pv_table[state_idx].alpha << '\n';
          std::cerr << "Retrieving move " << (int)pv_table[state_idx].move.from << " " << (int)pv_table[state_idx].move.to << '\n';
        }
        */
        tt.save(state_key, 0, alpha, EXACT_BOUND, depth, depth);
        void save(uint64_t key, int move, int score, int bound, int depth, int ply);
        move_root = best_move;
        return beta;
      }
    }
  }

  if(empty_move(best_move)) return alpha;
  assert(!empty_move(best_move));

  history[position.side][best_move.from][best_move.to] += depth * depth;
  age_history();

  /*
  // std::cout << "Storing move " << str_move(best_move.from, best_move.to) << " at " << state_key << " at tt" << '\n';
  if(state_key == -7145734984389955738) {
    std::cerr << "Storing move with alpha = " << alpha << '\n';
    std::cerr << "Move is " << (int)best_move.from << " " << (int)best_move.to << '\n';
  }
  pv_table[state_idx] = PV_Entry(state_key, alpha, best_move);
  if(state_key == -7145734984389955738) {
    std::cerr << "Retrieving " << pv_table[state_idx].alpha << '\n';
  }
  */
  tt.save(state_key, 0, alpha, EXACT_BOUND, depth, depth);
  move_root = best_move;

  return alpha;
}

int quiescence_search(Position& position, int alpha, int beta) {
  stats.change_phase(Q_SEARCH);

  int score = eval_tscp(position);
  if(score >= beta) {
    return beta;
  } else if(score > alpha) {
    alpha = score;
  }

  int first_move = (int)move_stack.size();
  generate_capture_moves(position);
  int last_move = (int)move_stack.size() - 1;

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    position.make_move(move.from, move.to, QUEEN);
    taken_moves.push_back(move);
    score = -quiescence_search(position, -beta, -alpha);
    position.take_back(move);
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
  float elapsed_time = stats.elapsed_time(); 
  if(elapsed_time >= max_search_time) {
    if(depth <= 4) return false; // we want to search at least depth 4
    stop_search = true;
    return true;
  }
  float threshold_time = float(printed_points) * (max_search_time / 80.0);
  if(elapsed_time >= threshold_time) {
    std::cout << ".";
    std::cout.flush();
    printed_points++;
  }  
  return false;
}
