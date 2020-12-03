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

namespace {
    int printed_points;
    int max_depth_searched;
    Move move_root;
    bool stop_search;
	Position initial_position;
}

void age_history() {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 64; j++) {
            for (int k = 0; k < 64; k++) {
                history[i][j][k] = history[i][j][k] >> 1;
            }
        }
    }
}

bool timeout() {
    float elapsed_time = stats.elapsed_time();
    /*
    std::cout << "Elapsed time is " << elapsed_time << ", max search time is " << MAX_SEARCH_TIME << endl;
    std::cout << "Max depth searched is " << max_depth_searched << endl;
    std::cout << "Stop search is " << stop_search << endl;
    std::cout << endl;
    */
    if (elapsed_time >= MAX_SEARCH_TIME) {
        if (max_depth_searched <= 4) {
            std::cout << "Max depth searched not accomplished yet" << endl;
            return false; // we want to search at least depth 4
        }
        stop_search = true;
        return true;
    }
#ifndef SELF_PLAY
    float threshold_time = float(printed_points) * (MAX_SEARCH_TIME / 80.0);
    if (elapsed_time >= threshold_time) {
        std::cout << ".";
        std::cout.flush();
        printed_points++;
    }	
#endif
    return false;
}

int quiescence_search(Position & position, int alpha, int beta, int depth) {
    stats.change_phase(Q_SEARCH);

    int score = eval_tscp(position);
    if (score >= beta) {
        return beta;
    } else if (score > alpha) {
        alpha = score;
    }

    if (depth == 0) {
        return alpha;
    }

    int first_move = (int) move_stack.size();
    generate_capture_moves(position);
    int last_move = (int) move_stack.size() - 1;

	Position prev_position = position;

    for (int i = last_move; i >= first_move; i--) {
        Move move = move_stack[i];
        position.make_move(move);
        taken_moves.push_back(move);
        score = -quiescence_search(position, -beta, -alpha, depth - 1);
		position = prev_position;
        taken_moves.pop_back();
        move_stack.pop_back();

        if (stop_search) {
            return alpha;
        }
        if (score > alpha) {
            alpha = score;
            if (beta <= alpha) {
                while (i--> first_move) move_stack.pop_back();
                return beta;
            }
        }
    }

    return alpha;
}

int search(Position & position, int alpha, int beta, int depth, bool is_root = false) {
    stats.change_phase(SEARCH);

    if (timeout()) {
        return alpha;
    }

    if (position.in_check(position.side)) {
        depth++;
    } else if (depth == 0) {
        int score = quiescence_search(position, alpha, beta, 4);
        return score;
    }

    // we check if search has already been performed for this state
    long long state_key = get_hash(position);

    Move retrieved_move = NULL_MOVE;
    int retrieved_score = 0;
    int retrieved_flags = 0;

    if (!is_root && tt.retrieve_data(state_key, retrieved_move, retrieved_score, retrieved_flags, alpha, beta, depth, 0)) {
        if(position.move_valid(move_root)) {
            return std::max(alpha, retrieved_score);
        }
    }

    int first_move = (int) move_stack.size();
    generate_moves(position); // moves are already sorted
    int last_move = (int) move_stack.size() - 1;

    // no moves were generated
    if (first_move == last_move + 1) {
        return position.in_check(position.side) ? -99999 : 0;
    } else if (position.is_draw()) { 
        return 0;
    }

    Move best_move = NULL_MOVE;
	Position prev_position = position;

    for (int i = last_move; i >= first_move; i--) {
        Move move = move_stack[i];
		assert(position.move_valid(move));
        position.make_move(move);
        taken_moves.push_back(move);
        bool book_state_before = book_deactivated;
        int score = -search(position, -beta, -alpha, depth - 1);
		position = prev_position;
        if(is_root && !position.same(initial_position)) {
            throw("Error when taking back during search, position is different than initial");
        }
        taken_moves.pop_back();
        move_stack.pop_back();
        book_deactivated = book_state_before;
        if (stop_search) {
            return alpha;
        }
        // move increases the alpha-cutoff
        if (score > alpha) {
            best_move = move;
            alpha = score;
            if (score >= beta) {
                history[position.side][from(move)][to(move)] += depth * depth;
                age_history();
                // the move caused a beta-cutoff so it must be good
                // but it won't be picked by parent
                while (i-- > first_move) move_stack.pop_back();
                tt.save(state_key, best_move, beta, LOWER_BOUND, depth, 0);
                if(!position.move_valid(best_move)) {
                    throw("Generated and selected move is invalid");
                }
                if(is_root) {
                    throw("There shouldn't be any beta cutoff at root");
                } 
                return beta;
            }
        }
    }

    if(alpha == -999999) {
        throw("Search can't find any move better than minimum score");
    }

	if(best_move == NULL_MOVE) {
        if(is_root) {
            throw("Root didn't find any good move");
        } else {
            return alpha;
        }
	}

    history[position.side][from(best_move)][to(best_move)] += depth * depth;
    age_history();

    tt.save(state_key, best_move, alpha, EXACT_BOUND, depth, 0);
	assert(position.move_valid(best_move));

    if(is_root && !position.same(initial_position)) {
        throw("Position at root after search differs from initial position");
    }

    if(is_root) {
        move_root = best_move;
    }

    return alpha;
}

Move think(Position& position) {
#ifndef SELF_PLAY
    // get a move from the book if we can
    Move book_move = get_book_move();
    if (book_move != NULL_MOVE) {
        std::cout << "Returning book move" << '\n';
        return book_move;
    } else {
        std::cout << "No book move available" << '\n';
    }
#endif
    // reset statistics and vars
    stats.init();
    stop_search = false;
    printed_points = 0;
    Move move_root_non_timeout = NULL_MOVE;
    move_root = NULL_MOVE;
    age_history();
	initial_position = position;
    // we search iteratively with increasing depth until we run out of time
    for (max_depth_searched = 4; max_depth_searched <= MAX_DEPTH && !stop_search;) {
        search(position, -999999, 999999, max_depth_searched, true);
		if(stop_search) {
			max_depth_searched -= 2;
			break;
		}
		assert(position.same(initial_position));
		move_root_non_timeout = move_root;
		if(!position.move_valid(move_root_non_timeout)) {
            std::cout << endl << "Move is: " << int(move_root_non_timeout) << endl;
            std::cout << "Board is:" << endl;
            position.print_board();
            throw("Result of search is an invalid move"); // + move_to_str(move_root_non_timeout));
		}
		max_depth_searched += 2;
    }
    // in case that move_root was assigned after timeout
    move_root = move_root_non_timeout;
#ifndef SELF_PLAY
    std::cout << endl; // .....
    std::cout << "Searched a maximum depth of: " << max_depth_searched << endl;
    std::cout << "Best move is: " << move_to_str(move_root) << endl;
    stats.display();
    std::cout << "................................................................................" << endl << endl;
#endif
    while (!move_stack.empty()) {
        move_stack.pop_back();
    }
    return move_root;
}
