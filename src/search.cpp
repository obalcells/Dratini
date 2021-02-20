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
        int value = search(thread, pv, alpha, beta, thread.depth);
        assert(thread.ply == 0);
        
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
    bool is_root = thread.ply == 0;    
    bool is_pv = (alpha != beta - 1);

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
        if(elapsed_time() >= MAX_SEARCH_TIME) {
            stop_search = true;
        }
    }

    if(stop_search) { 
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
    Move tt_move = NULL_MOVE, best_move, move;
    std::vector<Move> captures_tried, quiets_tried;
    int score, best_score = -CHECKMATE, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, depth, thread.ply
    )) {
       return tt_score; 
    }

    if(in_check) {
        depth++;
    }

    int eval_score = eval_tscp(thread.board);

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
        thread.board.error_check();
        thread.move_stack.push_back(NULL_MOVE);
        thread.ply++;

        int null_move_value = -search(thread, child_pv, -beta, -beta + 1, depth - 3);
        assert(thread.move_stack.back() == NULL_MOVE);
        thread.move_stack.pop_back();
        thread.board.take_back(undo_data);
        thread.board.error_check();
        thread.ply--;

        if(null_move_value >= beta) {
            return beta;
        }
    }

    MovePicker move_picker = MovePicker(thread, tt_move);

    while(true) {
        move = move_picker.next_move();

        if(move == NULL_MOVE || stop_search) {
            break;
        }

        if(!thread.board.move_valid(move)) {
            continue;
        }

        if(get_flag(move) == QUIET_MOVE && thread.board.color_at[get_to(move)] != EMPTY) {
            cerr << "A quiet move was generated, which is actually capturing a piece" << endl;
            cerr << "Move is: " << move_to_str(move) << endl;
            cerr << "Ply is " << thread.ply << endl;
            cerr << "Thread board is:" << endl;
            thread.board.print_board();
            cerr << "MovePicker board is:" << endl;
            move_picker.board->print_board();
            assert(thread.board.get_piece(get_to(move)) == EMPTY);
            assert(thread.board.color_at[get_to(move)] == EMPTY);
            assert(move_picker.board->get_piece(get_to(move)) == EMPTY);
            assert(move_picker.board->color_at[get_to(move)] == EMPTY);
        }
        assert(get_flag(move) != QUIET_MOVE || thread.board.color_at[get_to(move)] == EMPTY);

        if(thread.board.color_at[get_from(move)] != thread.board.side) {
            cerr << "The following move is being done from the wrong side: " << move_to_str(move) << endl;
            cerr << "Ply is: " << thread.ply << endl;
            cerr << "Board at the moment is: " << endl;
            thread.board.print_board();
            cerr << "Color at that position is " << (int)thread.board.color_at[get_from(move)] << endl;
        }
        assert(thread.board.color_at[get_from(move)] == thread.board.side);

        UndoData undo_data = thread.board.make_move(move);
        thread.board.error_check();
        thread.move_stack.push_back(move);
        thread.ply++;

        if(get_flag(move) == QUIET_MOVE || get_flag(move) == CASTLING_MOVE) {
            quiets_tried.push_back(move);
        } else {
            captures_tried.push_back(move);
        }

        if(best_score == -CHECKMATE) {
            score = -search(thread, child_pv, -beta, -alpha, depth - 1); 
        } else {
            score = -search(thread, child_pv, -alpha - 1, -alpha, depth - 1);

            if(!stop_search && score > alpha && score < beta) {
                score = -search(thread, child_pv, -beta, -alpha, depth - 1);
            }
        }

        thread.board.take_back(undo_data);
        thread.board.error_check();
        thread.move_stack.pop_back();
        thread.ply--;

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

                if(is_root) {
                    for(int i = 0; i < (int)pv.size(); i++) {
                        cout << move_to_str(pv[i]) << " ";
                    }
                    cout << endl;
                }

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
    int score, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, 0, thread.ply
    )) {
       return tt_score; 
    }

    PV child_pv;
    Move best_move;
    int best_score = tt_bound != -1 ? tt_score : eval_tscp(thread.board);

    // eval pruning
    alpha = std::max(alpha, best_score);
    if(alpha >= beta) {
        return alpha;
    }

    MovePicker move_picker(thread, tt_move, true);

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
