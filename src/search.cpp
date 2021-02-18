#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>
#include <sys/timeb.h>

#include "defs.h"
#include "board.h"
#include "book.h"
#include "eval_tscp.h"
#include "tt.h"
#include "stats.h"
#include "move_picker.h"
#include "position.h"
#include "search.h"

static bool stop_search;
static std::chrono::time_point<std::chrono::system_clock> initial_time;

float elapsed_time() {
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::milli> duration = time_now - initial_time;
    return duration.count();
}

// extremely dumb think function
Move think(const Position& position) {
    // cerr << "Before initializing thread" << endl;

    Thread main_thread = Thread(position);
    initial_time = std::chrono::system_clock::now(); 
    stop_search = false;
    
    // cerr << "Thread initialized" << endl;

    for(main_thread.depth = 1; main_thread.depth <= MAX_PLY; main_thread.depth++) {
            //MAX_PLY; main_thread.depth++) {
        aspiration_window(main_thread);
    }

    assert(main_thread.best_move != NULL_MOVE);
    return main_thread.best_move;
}

void aspiration_window(Thread& thread) {
    int alpha = -CHECKMATE;
    int beta = CHECKMATE;
    int delta = INITIAL_WINDOW_SIZE;
    PV pv;

    if(thread.depth >= MIN_DEPTH_FOR_WINDOW) {
        alpha = std::max(-CHECKMATE, thread.root_value - delta); 
        beta = std::min(CHECKMATE, thread.root_value + delta);
    }

    while(!stop_search) {
        // cerr << "Searching " << alpha << " " << beta << " from aspiration window with depth " << thread.depth << endl;
        cerr << RED_COLOR << "Side at the beginning is " << (thread.position.get_board().side == WHITE ? "WHITE" : "BLACK") << RESET_COLOR << endl;
        int value = search(thread, pv, alpha, beta, thread.depth);
        // cerr << GREEN_COLOR << "Finished searching" << RESET_COLOR << endl;
        assert(thread.position.get_board().color_at[get_from(pv[0])] == thread.position.get_board().side);
        
        if(value > beta) {
           beta = std::min(CHECKMATE, beta + delta); 
        } else if(value < alpha) {
            beta = (alpha + beta) / 2;
            alpha = alpha - delta;
        } else {
            thread.root_value = value;
            assert(pv.size() > 0);
            thread.best_move = pv[0];
            // cerr << "Best move is " << move_to_str(pv[0]) << endl;
            thread.ponder_move = pv.size() > 1 ? pv[1] : NULL_MOVE;
            return;
        }

        delta = delta + delta / 2;
    }
}

int search(Thread& thread, PV& pv, int alpha, int beta, int depth) {
    cerr << endl << endl;
    cerr << MAGENTA_COLOR << "Beginning of search" << RESET_COLOR << endl;
    cerr << endl << endl;

    bool is_root = thread.ply == 0;    
    bool is_pv = (alpha != beta - 1);

    assert(thread.ply >= 0);

    // cerr << BLUE_COLOR << "Depth is " << depth << RESET_COLOR << endl;

    // cerr << GREEN_COLOR << (board.side ? "WHITE" : "BLACK") << RESET_COLOR << endl;

    // early exit conditions
    if(!is_root) {
        if(thread.position.get_board().is_draw()) {
            return thread.nodes & 2;
        } 
        if(thread.ply >= MAX_PLY) {
            return eval_tscp(thread.position.get_board());
        }
    }

    if((thread.nodes & 1023) == 0) {
        // cerr << thread.nodes << " " << elapsed_time() << endl;
        if(elapsed_time() >= MAX_SEARCH_TIME) {
            stop_search = true;
        }
    }

    // cerr << 3.5 << endl;

    if(stop_search) { 
        // cerr << "Stopping search" << endl;
        return 0;
    }

    thread.nodes++;

    // cerr << 1 << " " << depth << endl;
        
    if(depth <= 0 && !thread.position.get_board().in_check()) {
        return q_search(thread, pv, alpha, beta);
    }

    // cerr << 2 << endl;

    // cerr << 3 << endl;

    pv.clear();
    thread.killers[thread.ply + 1][0] = NULL_MOVE;
    thread.killers[thread.ply + 1][1] = NULL_MOVE;

    // cerr << 4 << endl;

    PV child_pv;
    Move tt_move = NULL_MOVE, best_move, move;
    std::vector<Move> captures_tried, quiets_tried;
    int best_score = -CHECKMATE, tt_score, tt_bound = -1;

    Board board_at_beginning = thread.position.board_history.back(); // get_board();
    
    // cerr << 5 << endl;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve_data(
        thread.position.get_board().key, tt_move,
        tt_score, tt_bound, alpha, beta, depth, thread.ply
    )) {
       return tt_score; 
    }
    
    // cerr << 6 << endl;

    bool in_check = thread.position.in_check();

    if(in_check) {
        depth++;
    }

    // cerr << 7 << endl;

    int eval_score = eval_tscp(thread.position.get_board());

    // cerr << 8 << endl;

    // beta pruning
    if(!is_pv
    && !in_check
    && depth <= MIN_BETA_PRUNING_DEPTH
    && eval_score - BETA_MARGIN * depth > beta) {
        return eval_score;
    }

    // null move pruning
    if(false
    && !is_pv
    && !in_check 
    && eval_score >= beta
    && depth >= MIN_NULL_MOVE_PRUNING_DEPTH
    && thread.move_stack[thread.ply - 1] != NULL_MOVE
    && (tt_bound == -1 || tt_score >= beta || tt_bound != UPPER_BOUND)) {
        thread.position.make_move(NULL_MOVE);
        thread.move_stack.push_back(NULL_MOVE);
        thread.ply++;

        int null_move_value = -search(thread, child_pv, -beta, -beta + 1, depth - 3);
        thread.move_stack.pop_back();
        thread.position.take_back();
        thread.ply--;

        if(null_move_value >= beta) {
            return beta;
        }
    }

    cerr << MAGENTA_COLOR << "Initially, the board looks like this " << thread.ply << endl;
    thread.position.get_board().print_board();
    cerr << endl << endl << RESET_COLOR;

    // we know this is constant
    Board initial_board = thread.position.board_history.back();

    cerr << "Initial board is " << thread.ply << endl;
    initial_board.print_board();

    MovePicker move_picker(thread, tt_move, initial_board);

    while(true) {
        assert(board_at_beginning == initial_board);
        assert(board_at_beginning == *move_picker.board);

        move = move_picker.next_move();

        assert(board_at_beginning == initial_board);
        assert(board_at_beginning == *move_picker.board);

        if(move == NULL_MOVE || stop_search) {
            break;
        }

        if(!thread.position.move_valid(move)) {
            continue;
        }

		if(move_to_str(move) == "a6c5") {
			cerr << BLUE_COLOR << "After reading the following move " << move_to_str(move) << endl;
			cerr << "Board looks like this " << thread.ply << endl;
			thread.position.get_board().print_board();
            cerr << "Initial board now looks like this " << thread.ply << endl;
            initial_board.print_board();
			cerr << RESET_COLOR;
		}

        if(thread.position.get_board().color_at[get_from(move)] != thread.position.get_board().side) {
            cerr << "The following move is being done from the wrong side: " << move_to_str(move) << endl;
            cerr << "Board at the moment is " << thread.ply << endl;
            thread.position.get_board().print_board();
            assert(*move_picker.board == thread.position.get_board());
            assert(board_at_beginning == thread.position.get_board());
            cerr << "Color at that position is " << (int)thread.position.get_board().color_at[get_from(move)] << endl;
            assert(false);
        }

        bool color_before_move = thread.position.get_board().side;

        Board board_before_move = thread.position.get_board();

        thread.position.make_move(move);
        thread.move_stack.push_back(move);
        thread.ply++;

        bool color_after_move = thread.position.get_board().side;

        if(get_flag(move) == QUIET_MOVE || get_flag(move) == CASTLING_MOVE) {
            quiets_tried.push_back(move);
        } else {
            captures_tried.push_back(move);
        }

        int score;

        cerr << GREEN_COLOR << "Starting search " << thread.ply << endl << RESET_COLOR;

        if(best_score == -CHECKMATE) {
            score = -search(thread, child_pv, -beta, -alpha, depth - 1); 
        } else {
            score = -search(thread, child_pv, -alpha - 1, -alpha, depth - 1);

            if(!stop_search && score > alpha && score < beta) {
                score = -search(thread, child_pv, -beta, -alpha, depth - 1);
            }
        }

        cerr << GREEN_COLOR << "Finished search " << thread.ply << endl << RESET_COLOR;
        // cerr << "XXX" << endl;

        thread.position.take_back();
        thread.move_stack.pop_back();
        thread.ply--;

        assert(board_before_move == thread.position.get_board());

        bool color_after_taking_back = thread.position.get_board().side;

        if(color_before_move == color_after_move || color_before_move != color_after_taking_back) {
            cerr << "There was a problem with the side when making the following move " << move_to_str(move) << endl;
            cerr << "Position is the following:" << endl;
            thread.position.get_board().print_board();
        }  

        // cerr << "YYY" << endl;

        if(score > best_score) {
            best_score = score;
            best_move = move;

            // cerr << "ZZZ" << endl;

            if(score > alpha) {
                alpha = score;

                // cerr << "WWW" << endl;

                pv.clear();
                pv.push_back(move);
                for(int i = 0; i < (int)child_pv.size(); i++) {
                    pv.push_back(child_pv[i]);
                }
                // pv.insert(pv.begin(), child_pv.begin(), child_pv.end());

                cerr << "PV update: (color now is " << (thread.position.get_board().side == WHITE ? "white" : "black") << ")" << endl; 
                for(int i = 0; i < (int)pv.size(); i++) {
                    cerr << move_to_str(pv[i]) << " ";
                }
                cerr << endl;

                // cerr << "Me cago en la puta " << thread.ply << " " << is_root << " " << move_to_str(move) << endl;

                if(alpha >= beta) {
                    break;
                }
            }
        }
    }

    // cerr << 8 << endl;

    if(best_score == -CHECKMATE) {
        return in_check ? -CHECKMATE + thread.ply : 0;
    }

    // cerr << 9 << endl;

    if(best_score >= beta && get_flag(best_move) != CAPTURE_MOVE) {
        update_quiet_history(thread, best_move, quiets_tried, depth);
    } 
    
    // cerr << 10 << endl;

    if(best_score >= beta) {
        update_capture_history(thread, best_move, captures_tried, depth); 
    }

    // cerr << 11 << endl;

    if(best_score >= beta) {
        tt.save(
            thread.position.get_board().key, best_move, best_score,
            LOWER_BOUND, depth, thread.ply
        );
    } else if(best_score <= alpha) {
        tt.save(
            thread.position.get_board().key, best_move, alpha,
            UPPER_BOUND, depth, thread.ply
        );
    } else {
        tt.save(
            thread.position.get_board().key, best_move, best_score,
            EXACT_BOUND, depth, thread.ply
        );
    }

    // cerr << 12 << " " << thread.ply << endl;

    return best_score;
}

int q_search(Thread& thread, PV& pv, int alpha, int beta) {
    pv.clear();
    thread.nodes++;

    // cerr << "q 1" << endl;

    if((thread.nodes & 1023) == 0 && elapsed_time() >= MAX_SEARCH_TIME) {
        stop_search = true;
    }

    // cerr << "q 2" << endl;

    if(stop_search) {
        return 0;
    }

    // check early exit conditions
    if(thread.position.get_board().is_draw()) {
        return thread.nodes & 2;
    }

    // cerr << "q 3" << endl;

    if(thread.ply >= MAX_PLY) {
        return eval_tscp(thread.position.get_board());
    }

    Move tt_move = NULL_MOVE;
    int tt_score, tt_bound = -1;

    // cerr << "q 4" << endl;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve_data(
        thread.position.get_board().key, tt_move,
        tt_score, tt_bound, alpha, beta, 0, thread.ply
    )) {
       return tt_score; 
    }

    // cerr << "q 5" << endl;
    
    PV child_pv;
    Move best_move;
    // std::vector<Move> captures_tried;
    int eval_score = tt_bound != -1 ? tt_score : eval_tscp(thread.position.get_board());
    int best_score = eval_score;

    // eval pruning
    alpha = std::max(alpha, best_score);
    if(alpha >= beta) {
        return alpha;
    }

    // cerr << "q 6" << endl;

    Board initial_board = thread.position.board_history.back();
    
    MovePicker move_picker(thread, tt_move, initial_board, true);

    while(true) {
        Move move = move_picker.next_move(); 

        if(stop_search || move == NULL_MOVE) {
            break;
        }

        if(!thread.position.move_valid(move)) {
            continue;
        }

        thread.position.make_move(move);
        thread.ply++;

        int score = -q_search(thread, child_pv, -beta, -alpha);

        thread.position.take_back();
        thread.ply--;

        if(score > best_score) {
            best_score = score;
            best_move = move;

            if(best_score > alpha) {
                alpha = best_score;

                pv.clear();
                pv.push_back(move);
                pv.insert(pv.end(), child_pv.begin(), child_pv.end());

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
    const int side = thread.position.get_board().side;

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
        const int piece = thread.position.get_board().piece_at[from];
        int captured = thread.position.get_board().piece_at[to];

        if(flag == NULL_MOVE || flag == QUIET_MOVE || flag == CASTLING_MOVE) {
            // cerr << "Bad flag in capture moves " << flag << endl;
        }

        assert(flag != NULL_MOVE && flag != QUIET_MOVE && flag != CASTLING_MOVE);

        if(flag != CAPTURE_MOVE) {
            captured = PAWN;
        }
        
        if(!(captured >= PAWN && captured <= KING)) {
            // cerr << "Incorrect captured " << captured;
        }

        assert(captured >= PAWN && captured <= KING);

        int entry = thread.capture_history[piece][to][captured];
        entry += HISTORY_MULTIPLIER * delta - entry * std::abs(delta) - HISTORY_DIVISOR;
        thread.capture_history[piece][to][captured] = entry;
    }
}
