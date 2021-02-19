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
Move think(const Board& board) {
    // cerr << "Before initializing thread" << endl;

    Thread main_thread = Thread(board);
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
        cerr << RED_COLOR << "Side at the beginning is " << (thread.board.side == WHITE ? "WHITE" : "BLACK") << RESET_COLOR << endl;
        int value = search(thread, pv, alpha, beta, thread.depth);
        assert(thread.ply == 0);
        // cerr << GREEN_COLOR << "Finished searching" << RESET_COLOR << endl;
        // assert(thread.board.color_at[get_from(pv[0])] == thread.board.side);
        
        if(value >= beta) {
           beta = std::min(CHECKMATE, beta + delta); 
        } else if(value <= alpha) {
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
    cerr << endl;
    cerr << MAGENTA_COLOR << "Beginning of search" << RESET_COLOR << endl;
    cerr << endl;

    bool is_root = thread.ply == 0;    
    bool is_pv = (alpha != beta - 1);

    assert(thread.ply >= 0);

    // cerr << BLUE_COLOR << "Depth is " << depth << RESET_COLOR << endl;
    // cerr << GREEN_COLOR << (board.side ? "WHITE" : "BLACK") << RESET_COLOR << endl;

    // early exit conditions
    if(!is_root) {
        if(thread.board.is_draw()) {
            return thread.nodes & 2;
        } 
        if(thread.ply >= MAX_PLY) {
            return eval_tscp(thread.board);
        }
    }

    if((thread.nodes & 1023) == 0) {
        // cerr << thread.nodes << " " << elapsed_time() << endl;
        if(elapsed_time() >= MAX_SEARCH_TIME) {
            stop_search = true;
        }
    }

    if(stop_search) { 
        return 0;
    }

    thread.nodes++;

    if(depth <= 0 && !thread.board.in_check()) {
        return q_search(thread, pv, alpha, beta);
    }

    pv.clear();
    thread.killers[thread.ply + 1][0] = NULL_MOVE;
    thread.killers[thread.ply + 1][1] = NULL_MOVE;

    PV child_pv;
    Move tt_move = NULL_MOVE, best_move, move;
    std::vector<Move> captures_tried, quiets_tried;
    int best_score = -CHECKMATE, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, depth, thread.ply
    )) {
       return tt_score; 
    }
    
    cerr << 6 << endl;

    bool in_check = thread.board.in_check();

    if(in_check) {
        depth++;
    }

    cerr << 7 << endl;

    int eval_score = eval_tscp(thread.board);

    cerr << 8.6 << endl;

    // beta pruning
    if(!is_pv
    && !in_check
    && depth >= MIN_BETA_PRUNING_DEPTH
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
        UndoData undo_data = thread.board.make_move(NULL_MOVE);
        thread.move_stack.push_back(NULL_MOVE);
        thread.ply++;

        int null_move_value = -search(thread, child_pv, -beta, -beta + 1, depth - 3);
        assert(thread.move_stack.back() == NULL_MOVE);
        thread.move_stack.pop_back();
        thread.board.take_back(undo_data);
        thread.ply--;

        if(null_move_value >= beta) {
            return beta;
        }
    }

    cerr << MAGENTA_COLOR << "Initially, the board looks like this " << thread.ply << endl;
    thread.board.print_board();
    cerr << endl << endl << RESET_COLOR;

    // we know this is constant
    Board initial_board = thread.board;

    // assert(board_at_beginning == initial_board);

    cerr << "Initial board is " << thread.ply << endl;
    initial_board.print_board();

    MovePicker move_picker(thread, tt_move, initial_board);

    // bool debug_now = (thread.ply == 7 && initial_board.get_data() == "3 1 2 4 12 2 1 3 0 12 0 12 5 0 0 0 12 12 12 12 12 12 12 12 12 0 12 0 0 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 6 7 12 12 6 6 6 6 12 6 6 6 9 7 8 10 11 12 12 9 1106155485056437330 4 2 8 0 0 1 1 1 0");
    // 
    // if(debug_now) {
        // cerr << GREEN_COLOR << "Debugging now" << endl << RESET_COLOR;
    // }

    while(true) {
        // assert(board_at_beginning == initial_board);
        // assert(board_at_beginning == *move_picker.board);

        cerr << MAGENTA_COLOR << "Next move" << endl << RESET_COLOR;
        move = move_picker.next_move();
        cerr << MAGENTA_COLOR << "After next move" << endl << RESET_COLOR;

        // assert(board_at_beginning == initial_board);
        // assert(board_at_beginning == *move_picker.board);

        if(move == NULL_MOVE || stop_search) {
            break;
        }

        cerr << "Making move " << move_to_str(move) << endl;

        if(!thread.board.move_valid(move)) {
            continue;
        }

        if(false && thread.board.color_at[get_from(move)] != thread.board.side) {
            cerr << "The following move is being done from the wrong side: " << move_to_str(move) << endl;
            cerr << "Board at the moment is " << thread.ply << endl;
            thread.board.print_board();
            // assert(*move_picker.board == thread.board);
            // assert(board_at_beginning == thread.board);
            cerr << "Color at that position is " << (int)thread.board.color_at[get_from(move)] << endl;
            assert(false);
        }

        bool color_before_move = thread.board.side;

        const Board board_before_move = thread.board;

        UndoData undo_data = thread.board.make_move(move);
        thread.move_stack.push_back(move);
        thread.ply++;

        bool color_after_move = thread.board.side;

        if(get_flag(move) == QUIET_MOVE || get_flag(move) == CASTLING_MOVE) {
            quiets_tried.push_back(move);
        } else {
            captures_tried.push_back(move);
        }

        int score;

        if(best_score == -CHECKMATE) {
            score = -search(thread, child_pv, -beta, -alpha, depth - 1); 
        } else {
            score = -search(thread, child_pv, -alpha - 1, -alpha, depth - 1);

            if(!stop_search && score > alpha && score < beta) {
                score = -search(thread, child_pv, -beta, -alpha, depth - 1);
            }
        }

        thread.board.take_back(undo_data);
        thread.move_stack.pop_back();
        thread.ply--;

        // assert(!thread.board.error_check());

        // assert(board_before_move == thread.board);

        bool color_after_taking_back = thread.board.side;

        if(color_before_move == color_after_move || color_before_move != color_after_taking_back) {
            cerr << "There was a problem with the side when making the following move " << move_to_str(move) << endl;
            cerr << "Position is the following:" << endl;
            thread.board.print_board();
            assert(color_before_move != color_after_move);
            assert(color_before_move == color_after_taking_back);
        }  

        if(score > best_score) {
            best_score = score;
            best_move = move;

            if(score > alpha) {
                alpha = score;

                pv.clear();
                pv.push_back(move);
                for(int i = 0; i < (int)child_pv.size(); i++) {
                    pv.push_back(child_pv[i]);
                }

                PV other_pv;
                other_pv.push_back(move);
                other_pv.insert(other_pv.end(), child_pv.begin(), child_pv.end());

                // they must be the same
                assert(pv.size() == other_pv.size());
                for(int i = 0; i < (int)pv.size(); i++) {
                    assert(pv[i] == other_pv[i]);
                }

                cerr << "PV update: (color now is " << (thread.board.side == WHITE ? "white" : "black") << ")" << endl; 
                for(int i = 0; i < (int)pv.size(); i++) {
                    cerr << move_to_str(pv[i]) << " ";
                }
                cerr << endl;

                assert(!pv.empty());

                if(alpha >= beta) {
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

    if(best_score >= beta) {
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

    return best_score;
}

int q_search(Thread& thread, PV& pv, int alpha, int beta) {
    pv.clear();
    thread.nodes++;

    if((thread.nodes & 1023) == 0 && elapsed_time() >= MAX_SEARCH_TIME) {
        stop_search = true;
    }

    if(stop_search) {
        return 0;
    }

    // check early exit conditions
    if(thread.board.is_draw()) {
        return thread.nodes & 2;
    }

    if(thread.ply >= MAX_PLY) {
        return eval_tscp(thread.board);
    }

    Move tt_move = NULL_MOVE;
    int tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, 0, thread.ply
    )) {
       return tt_score; 
    }

    PV child_pv;
    Move best_move;
    int eval_score = tt_bound != -1 ? tt_score : eval_tscp(thread.board);
    int best_score = eval_score;

    // eval pruning
    alpha = std::max(alpha, best_score);
    if(alpha >= beta) {
        return alpha;
    }

    // cerr << "q 6" << endl;

    Board initial_board = thread.board;
    
    MovePicker move_picker(thread, tt_move, initial_board, true);

    while(true) {
        Move move = move_picker.next_move(); 

        if(stop_search || move == NULL_MOVE) {
            break;
        }

        if(!thread.board.move_valid(move)) {
            continue;
        }

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
