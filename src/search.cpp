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
int quiescence_search(Position&, int, int, int);
bool timeout();

namespace {
  int printed_points;
  int max_depth_searched;
  Move move_root;
  bool stop_search;
}

Move think(Position& position) {
  // get a move from the book if we can
#ifndef SELF_PLAY
  Move book_move = get_book_move();
  if(!empty_move(book_move)) { 
    std::cout << "Returning book move" << '\n';
    return book_move;
  } else {
    std::cout << "No book move available" << '\n';
  }
#endif
  /* reset statistics and vars */
  stats.init();
  stop_search = false;
  printed_points = 0;
  Move move_root_non_timeout = Move(); 
  move_root = Move();
  age_history();
  int max_depth_searched;
  // we search iteratively with increasing depth until we run out of time
  for(max_depth_searched = 4; max_depth_searched <= MAX_DEPTH && !stop_search;) {
    search(position, -999999, 999999, max_depth_searched);
    if(!stop_search) {
      move_root_non_timeout = move_root;
      max_depth_searched += 2;
    } else if(stop_search) {
      max_depth_searched -= 2; // max-depth where we did full-search
    }
  }
  // in case that move_root was assigned after timeout
  move_root = move_root_non_timeout;
#ifndef SELF_PLAY
  std::cout << '\n'; // .....
  std::cout << "Searched a maximum depth of: " << max_depth_searched << endl;
  std::cout << "Best move is: " << str_move(move_root.from, move_root.to) << endl;
  stats.display();
  std::cout <<
  "................................................................................"
  << endl << endl;
#endif
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
    int score = quiescence_search(position, alpha, beta, 4);
    return score;
  }
  
  // we check if search has already been performed for this state
  long long state_key = get_hash(position);

  int retrieved_move = 0;
  int retrieved_score = 0;
  int retrieved_flags = 0;

  if(tt.retrieve_data(state_key, retrieved_move, retrieved_score, retrieved_flags, alpha, beta, depth, 0)) {
    move_root = Move(retrieved_move);
    // see if move is valid
    return std::max(alpha, retrieved_score);
  }

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
        tt.save(state_key, best_move.to_bits(), beta, LOWER_BOUND, depth, 0);
        move_root = best_move;
        return beta;
      }
    }
  }

  if(empty_move(best_move)) return alpha;
  assert(!empty_move(best_move));

  history[position.side][best_move.from][best_move.to] += depth * depth;
  age_history();

  tt.save(state_key, best_move.to_bits(), alpha, EXACT_BOUND, depth, 0);
  move_root = best_move;

  return alpha;
}

int quiescence_search(Position& position, int alpha, int beta, int depth) {
  stats.change_phase(Q_SEARCH);

  int score = eval_tscp(position);
  if(score >= beta) {
    return beta;
  } else if(score > alpha) {
    alpha = score;
  }

  if(depth == 0)
    return alpha;

  int first_move = (int)move_stack.size();
  generate_capture_moves(position);
  int last_move = (int)move_stack.size() - 1;

  for(int i = last_move; i >= first_move; i--) {
    Move move = move_stack[i];
    position.make_move(move.from, move.to, QUEEN);
    taken_moves.push_back(move);
    score = -quiescence_search(position, -beta, -alpha, depth - 1);
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
  if(elapsed_time >= MAX_SEARCH_TIME) {
    if(max_depth_searched <= 4) return false; // we want to search at least depth 4
    stop_search = true;
    return true;
  }
#ifndef SELF_PLAY
  float threshold_time = float(printed_points) * (MAX_SEARCH_TIME / 80.0);
  if(elapsed_time >= threshold_time) {
    std::cout << ".";
    std::cout.flush();
    printed_points++;
  }  
#endif
  return false;
}
