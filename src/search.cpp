#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>
#include <sys/timeb.h>

#include "defs.h"
#include "board.h"
#include "book.h"
// #include "eval_tscp.h"
#include "sungorus_eval.h"
#include "tt.h"
#include "stats.h"
#include "move_picker.h"
#include "search.h"

float elapsed_time();
bool move_gives_check(Board&, const Move);

// const float MAX_SEARCH_TIME = 10000; 
static int max_search_time = 5000; 
static int max_depth = MAX_PLY;
static const int futility_max_depth = 10;
static int futility_margin[futility_max_depth];
static const int futility_linear = 35;
static const int futility_constant = 100;
static const double LMR_constant = -1.75;
static const double LMR_coeff = 1.03;
static int LMR[64][64];
static std::chrono::time_point<std::chrono::system_clock> initial_time;

void think(Engine& engine) {
    engine.stop_search = false;
    max_search_time = engine.max_search_time;
    max_depth = engine.max_depth;
    Thread main_thread = Thread(engine.board, &engine.stop_search);
    initial_time = std::chrono::system_clock::now(); 
    tt.age();

    for(int depth = 0; depth < futility_max_depth; depth++) {
        futility_margin[depth] = futility_constant + futility_linear * depth;
    }

	for (int depth = 0; depth < 64; depth++) {
		for (int moves_searched = 0; moves_searched < 64; moves_searched++) {
            LMR[depth][moves_searched] = std::round(LMR_constant + LMR_coeff * log(depth + 1) * log(moves_searched + 1));
		}
	}

    // PV pv;
    // for(main_thread.depth = 1; !(*main_thread.stop_search) && main_thread.depth <= engine.max_depth; main_thread.depth += 1) {
    //     main_thread.root_value = search(main_thread, pv, -INF, INF, main_thread.depth);
    //     main_thread.best_move = pv[0];
    //     if(engine.stop_search) {
    //         break;
    //     }
    // }

    for(main_thread.depth = 1; !(*main_thread.stop_search) && main_thread.depth <= engine.max_depth; main_thread.depth += 1) {
        aspiration_window(main_thread);
        if(!(*main_thread.stop_search)) {
            // cout << "Max search time was " << max_search_time << endl;
            // cout << "Finished searching at depth = " << main_thread.depth << endl;
        }
    }

    assert(main_thread.best_move != NULL_MOVE);
    engine.search_time = elapsed_time();
    engine.nodes = main_thread.nodes;
    engine.best_move = main_thread.best_move;
    engine.score = main_thread.root_value;
    engine.ponder_move = main_thread.ponder_move;
}

void aspiration_window(Thread& thread) {
    int alpha = -CHECKMATE;
    int beta = CHECKMATE;
    int delta = INITIAL_WINDOW_SIZE;
    PV pv;

    if(thread.root_value != -1 && thread.depth >= MIN_DEPTH_FOR_WINDOW) {
        alpha = std::max(-CHECKMATE, thread.root_value - delta); 
        beta = std::min(CHECKMATE, thread.root_value + delta);
    }

    while(!(*thread.stop_search)) {
        // cout << "Searching with window = " << "(" << alpha << ", " << beta << "), depth = " << thread.depth << endl; 
        int score = search(thread, pv, alpha, beta, thread.depth);

        assert(thread.ply == 0);

        if(score > alpha && score < beta) {
            assert(pv.size() > 0);
            thread.root_value = score;
            thread.best_move = pv[0];
            thread.ponder_move = pv.size() > 1 ? pv[1] : NULL_MOVE;
            return;
        } else if(score >= CHECKMATE - MAX_PLY) {
            beta = CHECKMATE;
        } else if(score <= -CHECKMATE + MAX_PLY) {
            alpha = -CHECKMATE;
        } else if(score >= beta) {
           beta = std::min(CHECKMATE, beta + delta); 
           // thread.best_move = pv[0]; // bad idea?
        } else if(score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = alpha - delta;
        }
        delta = delta + delta / 2;
    }
}

void debug_node(const Thread& thread) {
    cout << "Moves taken to get here:";
    for(int i = 0; i < (int)thread.move_stack.size(); i++) {
        cout << " " << move_to_str(thread.move_stack[i]);
    }
    cout << endl;
}

int search(Thread& thread, PV& pv, int alpha, int beta, int depth) {
    bool is_root = thread.ply == 0;    
    bool is_pv = (alpha != beta - 1);

    bool debug_mode = false; // true; // false; // true; // false;

    if(debug_mode) {
        cout << MAGENTA_COLOR << "Debugging special position" << RESET_COLOR << endl;
        cout << "Alpha and beta are " << alpha << " " << beta << endl;
        cout << "Ply is " << thread.ply << " and depth left is " << depth << ", initial depth is " << thread.depth << endl;
        debug_node(thread);
        cout << "Key is " << thread.board.key << endl;
        thread.board.print_board(); 
    }

    // early exit conditions
    if(thread.board.is_draw()) {
        return thread.nodes & 2;
    } else if(thread.ply >= max_depth) {
        return evaluate(thread.board);
    }
    
    if((thread.nodes & 1023) == 0 && elapsed_time() >= max_search_time) {
        *thread.stop_search = true;
    }

    if(*thread.stop_search) { 
        return 0;
    }

    thread.nodes++;
    
    bool in_check = thread.board.in_check();

    if(depth <= 0 && !in_check) {
        return q_search(thread, pv, alpha, beta);
    }

    pv.clear();
    thread.killers[thread.ply + 1][0] = NULL_MOVE;
    thread.killers[thread.ply + 1][1] = NULL_MOVE;
    PV child_pv;
    Move tt_move = NULL_MOVE, best_move = NULL_MOVE, move = NULL_MOVE;
    std::vector<Move> captures_tried, quiets_tried;
    int score, best_score = -CHECKMATE, tt_score, tt_bound = -1, searched_moves = 0;

    // it will return true if it causes a cutoff or is an exact value
    if(false && tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, depth, thread.ply
    )) {
        pv.push_back(tt_move);
        // should we reset the age of the entry?
        return tt_score; 
    }

    int eval_score = evaluate(thread.board);

    // beta pruning
    if(!is_pv
    && !in_check
    && depth >= MIN_BETA_PRUNING_DEPTH
    && eval_score - BETA_MARGIN * depth > beta) {
        return eval_score;
    }

    // null move pruning
    if(!is_pv
    && !in_check 
    && eval_score >= beta
    && depth >= MIN_NULL_MOVE_PRUNING_DEPTH
    && thread.move_stack[thread.ply - 1] != NULL_MOVE
    && (tt_bound == -1 || tt_score >= beta || tt_bound != UPPER_BOUND)) {
        UndoData undo_data = thread.board.make_move(NULL_MOVE);
        thread.move_stack.push_back(NULL_MOVE);
        thread.ply++;

        int null_move_value = -search(thread, child_pv, -beta, -beta + 1, depth - 3);
        assert(thread.move_stack.back() == NULL_MOVE);
        thread.move_stack.pop_back();
        thread.board.take_back(undo_data);
        thread.ply--;

        if(null_move_value >= beta) {
            if(depth >= 7) { // verification search
                score = search(thread, child_pv, -beta, beta - 1, depth - 3);
                if(score >= beta) {
                    return score;
                }
            } else {
                return beta;
            }
        }
    }

    bool is_futile = (depth < futility_max_depth)
                  && (eval_score + futility_margin[std::max(0, depth)] < alpha);

    MovePicker move_picker = MovePicker(thread, tt_move);

    while(true) {
        // assert(thread.board.key == thread.board.calculate_key());
        move = move_picker.next_move();
        // assert(thread.board.key == thread.board.calculate_key());

        if(move == NULL_MOVE || *thread.stop_search) {
            break;
        }
        
        if(!is_pv
        && get_flag(move) == QUIET_MOVE
        && !in_check
        && is_futile
        && searched_moves > 0
        && !move_gives_check(thread.board, move)) {
            continue;
        }

        assert(thread.board.move_valid(move));
        assert(get_flag(move) != QUIET_MOVE || thread.board.color_at[get_to(move)] == EMPTY);
        assert(thread.board.color_at[get_from(move)] == thread.board.side);

        UndoData undo_data = thread.board.make_move(move);
        thread.move_stack.push_back(move);
        thread.ply++;

        int extended_depth = depth + ((is_pv && in_check) ? 1 : 0);
        
        if(searched_moves > 3) { // taken from Halogen (https://github.com/KierenP/Halogen)
			int reduction = LMR[std::min(63, std::max(0, depth))][std::min(63, std::max(0, searched_moves))];

            if(is_pv) {
                reduction++;
            }

			reduction = std::max(0, reduction);

            score = -search(thread, child_pv, -alpha - 1, -alpha, extended_depth - 1 - reduction);

			if (score <= alpha) {
                thread.board.take_back(undo_data);
                thread.move_stack.pop_back();
                thread.ply--;
				continue;
			}
        }

        if(get_flag(move) == QUIET_MOVE || get_flag(move) == CASTLING_MOVE) {
            quiets_tried.push_back(move);
        } else {
            captures_tried.push_back(move);
        }

        if(best_score == -CHECKMATE) {
            score = -search(thread, child_pv, -beta, -alpha, extended_depth - 1); 
            searched_moves++;
        } else {
            score = -search(thread, child_pv, -alpha - 1, -alpha, extended_depth - 1);
            if(!*thread.stop_search && score >= alpha && score < beta) {
                score = -search(thread, child_pv, -beta, -alpha, extended_depth - 1);
                searched_moves++;
            }
        }

        thread.board.take_back(undo_data);
        thread.move_stack.pop_back();
        thread.ply--;

        if(score > best_score) {
            best_score = score;
            best_move = move;

            if(score > alpha) {
                pv.clear();
                pv.push_back(move);
                for(int i = 0; i < (int)child_pv.size(); i++) {
                    pv.push_back(child_pv[i]);
                }

                if(false && is_root) {
                    cout << move_to_str(pv[0]);
                    for(int i = 0; i < (int)pv.size(); i++) {
                        cout << " " << move_to_str(pv[i]);
                    }
                    cout << endl;
                }

                if(debug_mode) {
                    cout << "At ply = " << thread.ply << " ";
                    cout << "printing pv: "; 
                    for(int i = 0; i < (int)pv.size(); i++) {
                        cout << move_to_str(pv[i]) << " ";
                    }
                    cout << endl;
                    cout << "Prev alpha was " << alpha << endl;
                    cout << "Now alpha is " << score << endl;
                    cout << "Beta is " << beta << endl;
                }

                alpha = score;

                assert(!pv.empty());

                if(alpha >= beta) {
                    if(debug_mode) {
                        cout << "Breaking from the search" << endl;
                    }
                    break;
                }
            }
        }
    }

    if(best_score == -CHECKMATE) {
        return in_check ? -CHECKMATE + thread.ply : 0; // checkmate or stalemate
    }

    if(best_score >= beta && get_flag(best_move) != CAPTURE_MOVE) {
        update_quiet_history(thread, best_move, quiets_tried, depth);
    } 

    if(best_score >= beta) {
        update_capture_history(thread, best_move, captures_tried, depth); 
    }

    if(true) {
        // do nothing
    } else if(best_score >= beta) {
        tt.save(
            thread.board.key, best_move, best_score,
            LOWER_BOUND, depth, thread.ply
        );
    } else if(best_score <= alpha) {
        tt.save(
            thread.board.key, best_move, alpha,
            UPPER_BOUND, depth, thread.ply
        );
    } else {
        tt.save(
            thread.board.key, best_move, best_score,
            EXACT_BOUND, depth, thread.ply
        );
    }

    if(debug_mode) {
        assert(pv.empty() || pv[0] == best_move);
        cout << "At Ply = " << thread.ply << ", depth = " << depth << endl;
        cout << "Alpha =  " << alpha << ", beta = " << beta << endl;
        cout << "Best move is " << move_to_str(best_move) << endl;
        cout << "Score is " << best_score << endl;
        cout << "Board is " << endl;
        thread.board.print_board();
        cout << BLUE_COLOR << "***********************************" << RESET_COLOR << endl;
    }

    return best_score;
}

int q_search(Thread& thread, PV& pv, int alpha, int beta) {
    pv.clear();
    thread.nodes++;

    if((thread.nodes & 1023) == 0 && elapsed_time() >= max_search_time) {
        *thread.stop_search = true;
    }

    if(*thread.stop_search) {
        return 0;
    }

    // check early exit conditions
    if(thread.board.is_draw()) {
        return thread.nodes & 2;
    }

    if(thread.ply >= MAX_PLY) {
        return evaluate(thread.board);
    }

    Move tt_move = NULL_MOVE;
    int score, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(false && tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, 0, thread.ply
    )) {
       return tt_score; 
    }

    PV child_pv;
    Move best_move;
    int best_score = tt_bound != -1 ? tt_score : evaluate(thread.board);

    // eval pruning
    alpha = std::max(alpha, best_score);
    if(alpha >= beta) {
        return alpha;
    }

    MovePicker move_picker(thread, tt_move, true);

    while(true) {
        Move move = move_picker.next_move(); 

        if(*thread.stop_search || move == NULL_MOVE) {
            break;
        }

        assert(thread.board.move_valid(move));

        UndoData undo_data = thread.board.make_move(move);
        thread.ply++;

        int score = -q_search(thread, child_pv, -beta, -alpha);

        thread.board.take_back(undo_data); 
        thread.ply--;

        if(score > best_score) {
            best_score = score;
            best_move = move;

            if(best_score > alpha) {
                alpha = best_score;

                pv.clear();
                pv.push_back(move);
                pv.insert(pv.end(), child_pv.begin(), child_pv.end());

                assert(!pv.empty());

                if(alpha >= beta) {
                    return alpha;
                }
            }
        }
    }

    return best_score;
}

void update_quiet_history(Thread& thread, const Move best_move, const std::vector<Move>& quiets_tried, const int depth) {
    // the best move is a quiet move
    if(thread.killers[thread.ply][0] != best_move) {
        thread.killers[thread.ply][1] = thread.killers[thread.ply][1];
        thread.killers[thread.ply][0] = best_move;
    }

    if(quiets_tried.size() == 1 && depth < 4) {
        return;
    }

    const int bonus = -std::min(depth * depth, MAX_HISTORY_BONUS);
    const int side = thread.board.side;

    for(int i = 0; i < quiets_tried.size(); i++) {
        int delta = bonus;

        if(quiets_tried[i] == best_move) {
            delta = -bonus;
        }

        const int from = get_from(quiets_tried[i]);
        const int to = get_to(quiets_tried[i]); 

        int entry = thread.quiet_history[side][from][to];    
        entry += HISTORY_MULTIPLIER * delta - entry * std::abs(delta) / HISTORY_DIVISOR;
        thread.quiet_history[side][from][to] = entry;
    }
}

void update_capture_history(Thread& thread, const Move best_move, const std::vector<Move>& captures_tried, const int depth) {
    const int bonus = std::min(depth * depth, MAX_HISTORY_BONUS);

    for(int i = 0; i < captures_tried.size(); i++) {
        const int delta = captures_tried[i] == best_move ? bonus : -bonus;

        const int from = get_from(captures_tried[i]);
        const int to = get_to(captures_tried[i]);
        const int flag = get_flag(captures_tried[i]);
        const int piece = thread.board.piece_at[from];
        int captured = thread.board.piece_at[to];

        assert(flag != NULL_MOVE && flag != QUIET_MOVE && flag != CASTLING_MOVE);

        if(flag != CAPTURE_MOVE) {
            captured = PAWN;
        }
        
        assert(captured >= PAWN && captured <= KING);

        int entry = thread.capture_history[piece][to][captured];
        entry += HISTORY_MULTIPLIER * delta - entry * std::abs(delta) - HISTORY_DIVISOR;
        thread.capture_history[piece][to][captured] = entry;
    }
}

bool move_gives_check(Board& board, const Move move) {
    const UndoData undo_data = board.make_move(move);
    bool check_after_move = board.in_check();
    board.take_back(undo_data);
    return check_after_move;
}

float elapsed_time() {
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::milli> duration = time_now - initial_time;
    return int(duration.count()); // returns the elapsed time since search started in ms
}

