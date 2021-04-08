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
// #include "position.h"
#include "search.h"

// static bool stop_search;
// static bool interesting_reached;
static std::chrono::time_point<std::chrono::system_clock> initial_time;

float elapsed_time() {
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::milli> duration = time_now - initial_time;
    return duration.count();
}

void think(const Board& board, bool* engine_stop_search, Move& best_move, Move& ponder_move) {
    // cout << "Before initializing thread" << endl;

    Thread main_thread = Thread(board, engine_stop_search);
    initial_time = std::chrono::system_clock::now(); 
    // *main_thread.stop_search = false;

    // for(main_thread.depth = 6; !(*main_thread.stop_search) && main_thread.depth <= 6; main_thread.depth += 1) {
    for(main_thread.depth = 1; !(*main_thread.stop_search) && main_thread.depth <= MAX_PLY; main_thread.depth += 1) {
        aspiration_window(main_thread);
        // if(!(*main_thread.stop_search)) {
        //     cout << RED_COLOR << "Searched until depth " << main_thread.depth << endl << RESET_COLOR;
        // }
    }

    assert(main_thread.best_move != NULL_MOVE);
    best_move = main_thread.best_move;
    ponder_move = main_thread.ponder_move;
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
        // cout << "Starting search" << endl;
        // interesting_reached = false;
        // cout << "Searching with window = " << "(" << alpha << ", " << beta << "), depth = " << thread.depth << endl; 
        int value = search(thread, pv, alpha, beta, thread.depth);

        if(!(*thread.stop_search)) {
            // std::cout << BLUE_COLOR;
            // // std::cout << "After asp window with values " << alpha << ", " << beta
            // // << " with score " << value << endl;
            // // std::cout << "PV is:";
            // for(int i = 0; i < (int)pv.size(); i++) {
            //     std::cout << " " << move_to_str(pv[i]);
            // }
            // std::cout << endl;
            // std::cout << RESET_COLOR;
        } else {
            // std::cout << "Timeout!" << endl;
        }

        assert(thread.ply == 0);

        if(value > alpha && value < beta) {
            assert(pv.size() > 0);
            thread.root_value = value;
            thread.best_move = pv[0];
            thread.ponder_move = pv.size() > 1 ? pv[1] : NULL_MOVE;
            return;
        } else if(value >= CHECKMATE - MAX_PLY) {
            beta = CHECKMATE;
        } else if(value <= -CHECKMATE + MAX_PLY) {
            alpha = -CHECKMATE;
        } else if(value >= beta) {
           beta = std::min(CHECKMATE, beta + delta); 
           // thread.best_move = pv[0]; // bad idea?
        } else if(value <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = alpha - delta;
        }
        delta = delta + delta / 2;
    }
}

void debug_node(const Thread& thread) {
    cout << "Moves taken to get here:";
    for(int i = 0; i < (int)thread.move_stack.size(); i++) {
        if(thread.move_stack[i] == NULL_MOVE) {
            cout << " " << "0000";
        } else {
            cout << " " << move_to_str(thread.move_stack[i]);
        }
    }
    cout << endl;
}

int search(Thread& thread, PV& pv, int alpha, int beta, int depth) {
    bool is_root = thread.ply == 0;    
    bool is_pv = (alpha != beta - 1);

    // if(thread.board.piece_at[B6] == KING && thread.board.piece_at[D7] == QUEEN && thread.board.piece_at[B8] == KING && thread.ply == 5) {
    if(false && thread.board.piece_at[B6] == KING && thread.board.piece_at[D7] == QUEEN && thread.board.piece_at[A8] == KING && thread.ply == 6) {
        cout << BLUE_COLOR << "Key for the following position is " << thread.board.key << ", position was found at ply " << thread.ply << endl;
        thread.board.print_board();
        cout << RESET_COLOR;
    }

    bool debug_mode = 
       (thread.board.key == 1508007568069500693 && thread.ply == 0)
    || (thread.board.key == 957821848117016834 && thread.ply == 1)
    || (thread.board.key == 824685297147295837 && thread.ply == 2)
    || (thread.board.key == 1516646400408157036 && thread.ply == 3)
    || (thread.board.key == 1384635679415155251 && thread.ply == 4)
    || (thread.board.key == 1913318233855850599 && thread.ply == 5)
    || (thread.board.key == 404053874700670240 && thread.ply == 6);

    debug_mode = false;

    // bool debug_mode = 
    //    (thread.board.key == 1508007568069500693)
    // || (thread.board.key == 957821848117016834)
    // || (thread.board.key == 824685297147295837)
    // || (thread.board.key == 1516646400408157036)
    // || (thread.board.key == 1384635679415155251)
    // || (thread.board.key == 1913318233855850599)
    // || (thread.board.key == 404053874700670240);

    // alternative path
    debug_mode = debug_mode ||
        (thread.board.key == 1835259090973306315 && thread.ply == 4)
    ||  (thread.board.key == 1207642484593356703 && thread.ply == 5);

    debug_mode = false;

    // || (thread.board.key == 1362561496610412877 && thread.ply == 3)
    // || (thread.board.key == 2205877339120750570 && thread.ply == 4)
    // || (thread.board.key == 861132736221556449 && thread.ply == 5)
    // || (thread.board.key == 542481581466777670 && thread.ply == 6);
    // || (thread.board.key == 1359626067873678073 && thread.ply == 5)
    // || (thread.board.key == 2209665439142646878 && thread.ply == 6); 

    // if(thread.board.key == 542481581466777670 && thread.ply == 6) {
    //     interesting_reached = true;
    // }

    if(debug_mode) {
        cout << MAGENTA_COLOR << "Debugging special position" << RESET_COLOR << endl;
        cout << "Alpha and beta are " << alpha << " " << beta << endl;
        cout << "Ply is " << thread.ply << " and depth left is " << depth << ", initial depth is " << thread.depth << endl;
        debug_node(thread);
        cout << "Key is " << thread.board.key << endl;
        thread.board.print_board(); 
    }

    // cout << "At search with " << thread.ply << endl;
    // cout << thread.board.fifty_move_ply << endl;

    // early exit conditions
    if(!is_root) {
        // cout << "Pruning early" << endl;
        if(thread.board.is_draw()) {
            // cout << "It's a draw" << endl;
            // cout << "Board looks like this:" << endl;
            // thread.board.print_board();
            return thread.nodes & 2;
        } 
        if(thread.ply >= MAX_PLY) {
            // cout << "max ply reached" << endl;
            return eval_tscp(thread.board);
        }
        // cout << "Well, maybe not" << endl;
    }
    
    if(debug_mode) {
        cout << "Checkpoint 1" << endl;
    }

    if((thread.nodes & 1023) == 0 && elapsed_time() >= MAX_SEARCH_TIME) {
        *thread.stop_search = true;
    }

    if(*thread.stop_search) { 
        return 0;
    }

    thread.nodes++;
    
    bool in_check = thread.board.in_check();

    if(in_check) {
        depth++;
    }

    if(depth <= 0 && !in_check) {
        if(debug_mode) {
            // int q_score = q_search(thread, pv, alpha, beta);
            cout << "Side is " << (thread.board.side == WHITE ? "white" : "black") << endl; 
            cout << "X Side is " << (thread.board.xside == WHITE ? "white" : "black") << endl;
            int q_score = eval_tscp(thread.board);
            cout << "Q score is " << q_score << endl;
            cout << "Board is:" << endl;
            thread.board.print_board();
            // cout << BLUE_COLOR << "***********************************" << endl;
            return q_score;
        }   
        return q_search(thread, pv, alpha, beta);
        // return eval_tscp(thread.board);
    }

    if(debug_mode) {
        cout << "Checkpoint 2" << endl;
        cout << "Alpha is " << alpha << endl;
    }

    pv.clear();
    thread.killers[thread.ply + 1][0] = NULL_MOVE;
    thread.killers[thread.ply + 1][1] = NULL_MOVE;
    PV child_pv;
    Move tt_move = NULL_MOVE, best_move = NULL_MOVE, move = NULL_MOVE;
    std::vector<Move> captures_tried, quiets_tried;
    int score, best_score = -CHECKMATE, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(false && tt.retrieve_data(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, depth, thread.ply
    )) {
        pv.push_back(tt_move);
        return tt_score; 
    }

    int eval_score = eval_tscp(thread.board);

    // beta pruning
    if(false && !is_pv
    && !in_check
    && depth >= MIN_BETA_PRUNING_DEPTH
    && eval_score - BETA_MARGIN * depth > beta) {
        return eval_score;
    }

    if(debug_mode) {
        cout << "Checkpoint 3" << endl;
        cout << "Hasn't been beta-pruned " << alpha << endl;
    }

    // null move pruning
    if(!is_pv
    && !in_check 
    && eval_score >= beta
    && depth >= MIN_NULL_MOVE_PRUNING_DEPTH
    && thread.move_stack[thread.ply - 1] != NULL_MOVE
    && (tt_bound == -1 || tt_score >= beta || tt_bound != UPPER_BOUND)) {
        if(debug_mode) {
            cout << MAGENTA_COLOR << "We'll be null pruning?" << RESET_COLOR << endl;
        }
        bool prev_side_to_move = thread.board.side;
        UndoData undo_data = thread.board.make_move(NULL_MOVE);
        bool now_side_to_move = thread.board.side;
        assert(prev_side_to_move != now_side_to_move);
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
            // assert(!debug_mode);
            if(debug_mode) {
                int normal_search = search(thread, child_pv, -CHECKMATE, CHECKMATE, depth);
                cout << RED_COLOR << "We are null-move pruning a debug position " << endl;
                cout << "Alpha, beta and score: " << alpha << " " << beta << " " << null_move_value << endl;  
                cout << "Ply is " << thread.ply << " and depth left is " << depth << endl;
                cout << "Result from normal search would have been " << normal_search << endl;
                thread.board.print_board();
                cout << RESET_COLOR << endl;
            }
            // return null_move_value;
            return beta;
        }
    }

    MovePicker move_picker = MovePicker(thread, tt_move);

    while(true) {
        assert(thread.board.key == thread.board.calculate_key());
        move = move_picker.next_move();

        if(debug_mode) {
            cout << "Move " << move_to_str(move) << " returned by picker at ply " << thread.ply << endl;
        }

        assert(thread.board.key == thread.board.calculate_key());

        if(move == NULL_MOVE || *thread.stop_search) {
            if(debug_mode && *thread.stop_search) {
                cout << RED_COLOR << "Timeout!" << RESET_COLOR << endl;
            } else if(debug_mode) {
                thread.board.print_board();
                assert(move == NULL_MOVE);
                cout << RED_COLOR << "No moves left" << RESET_COLOR << endl;
                thread.board.print_board();
            }
            break;
        }

        assert(thread.board.move_valid(move));

        assert(get_flag(move) != QUIET_MOVE || thread.board.color_at[get_to(move)] == EMPTY);

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
            if(debug_mode) {
                cout << "Doing search for move " << move_to_str(move) << "with key " << thread.board.key << endl;
            }
            score = -search(thread, child_pv, -beta, -alpha, depth - 1); 
            if(debug_mode) {
                cout << "Score is " << score << endl;
            }
        } else {
            if(false && debug_mode) {
                cout << "Trying small search for move " << move_to_str(move) << " with key " << thread.board.key << endl;
            }
            score = -search(thread, child_pv, -alpha - 1, -alpha, depth - 1);
            // score = -search(thread, child_pv, -beta, -alpha, depth - 1);
            
            if(false && debug_mode) {
                cout << "Move returns score of " << score << " with alpha " << alpha << " after non-pv search" << endl;
                cout << "Here is his pv: " << move_to_str(move);
                for(int i = 0; i < (int)child_pv.size(); i++) {
                    cout << " " << move_to_str(child_pv[i]);
                }
                cout << endl;
            }

            if(!*thread.stop_search && score >= alpha && score < beta) {
                if(false && debug_mode) {
                    cout << "Doing full search" << endl;
                } 
                score = -search(thread, child_pv, -beta, -alpha, depth - 1);
            } else {
                if(false && debug_mode) {
                    cout << "Not doing full search" << endl;
                }
            }
        }

        thread.board.take_back(undo_data);
        thread.board.error_check();
        thread.move_stack.pop_back();
        thread.ply--;

        if(score > best_score) {
            if(debug_mode) {
                cout << "We found a better score!" << endl;
                cout << "Prev best move was " << move_to_str(best_move) << ", now it's " << move_to_str(move) << endl;
                cout << "Prev best score was " << best_score << ", now it's " << score << endl; 
            }

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
        if(false && in_check) {
            cout << GREEN_COLOR << "We found a checkmate " << RESET_COLOR << endl;
            cout << "Depth is " << depth << ", ply is " << thread.ply << ", initial depth is " << thread.depth << endl;
            cout << "Position is " << endl;
            thread.board.print_board();
            assert(false);
        }
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

    if(debug_mode) {
        // assert(alpha >= CHECKMATE - 10);
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

    if((thread.nodes & 1023) == 0 && elapsed_time() >= MAX_SEARCH_TIME) {
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
        return eval_tscp(thread.board);
    }

    Move tt_move = NULL_MOVE;
    int score, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(true
    && tt.retrieve_data(
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
