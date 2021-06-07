#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>
#include <sys/timeb.h>

#include "defs.h"
#include "board.h"
// #include "eval_tscp.h"
#include "sungorus_eval.h"
#include "tt.h"
#include "move_picker.h"
#include "new_move_picker.h"
#include "search.h"


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
static std::chrono::duration<float, std::milli> duration;

// returns elapsed time since search started in ms 
static inline int elapsed_time() {
    duration = std::chrono::system_clock::now() - initial_time;
    return int(duration.count());
}

void think(Engine& engine) {
    tt.total_saved = 0;
    tt.total_tried_save = 0;
    tt.totally_replaced = 0;
    engine.stop_search = false;
    max_search_time = engine.max_search_time;
    max_depth = engine.max_depth;
    assert(max_depth <= MAX_PLY);
    Thread main_thread = Thread(engine.board, &engine.stop_search);
    tt.age();

    for(int depth = 0; depth < futility_max_depth; depth++) {
        futility_margin[depth] = futility_constant + futility_linear * depth;
    }

	for (int depth = 0; depth < MAX_PLY; depth++) {
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

    initial_time = std::chrono::system_clock::now(); 
    for(main_thread.depth = 1; !(*main_thread.stop_search) && main_thread.depth <= engine.max_depth; main_thread.depth += 1) {
        aspiration_window(main_thread);
    }

    assert(main_thread.best_move != NULL_MOVE);
    engine.search_time = elapsed_time();
    engine.nodes = main_thread.nodes;
    engine.best_move = main_thread.best_move;
    engine.score = main_thread.root_value;
    engine.ponder_move = main_thread.ponder_move;
}

void aspiration_window(Thread& thread) {
    int alpha, beta, delta, depth, score;
    PV pv;
    pv.reserve(MAX_PLY);

    delta = INITIAL_WINDOW_SIZE;
    depth = thread.depth;

    if(thread.depth >= MIN_DEPTH_FOR_WINDOW) {
        alpha = std::max(-CHECKMATE, thread.root_value - delta); 
        beta = std::min(CHECKMATE, thread.root_value + delta);
    } else {
        alpha = -CHECKMATE;
        beta = CHECKMATE;
    }

    while(!(*thread.stop_search)) {
        score = search(thread, pv, alpha, beta, depth);

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
           depth = depth - (abs(score) <= CHECKMATE / 2);
           // thread.best_move = pv[0]; // bad idea?
        } else if(score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max(-CHECKMATE, alpha - delta);
            depth = thread.depth;
        }
        delta = delta + delta / 2;
    }
}

int search(Thread& thread, PV& pv, int alpha, int beta, int depth) {
    bool is_root = thread.ply == 0;    
    bool is_pv = (alpha != (beta - 1));
    bool debug_mode = false;

    if(debug_mode) {
        cout << MAGENTA_COLOR << "Debugging special position" << RESET_COLOR << endl;
        cout << "Alpha and beta are " << alpha << " " << beta << endl;
        cout << "Ply is " << thread.ply << " and depth left is " << depth << ", initial depth is " << thread.depth << endl;
        cout << "Key is " << thread.board.key << endl;
        thread.board.print_board(); 
    }

    if(thread.board.is_draw())
        return thread.nodes & 2;

    if(thread.ply >= max_depth)
        return evaluate(thread.board);
    
    if((thread.nodes & 4095) == 0 && elapsed_time() >= max_search_time)
        *thread.stop_search = true;

    if(*thread.stop_search)
        return 0;

    bool in_check = bool(thread.board.king_attackers);

    if(depth <= 0 && !in_check)
        return q_search(thread, alpha, beta);

    thread.nodes++;
    
    pv.clear();
    Move tt_move = NULL_MOVE;
    int tt_score = INF, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, depth, thread.ply
    )) {
        pv.push_back(tt_move);
        return tt_score;
    }

    thread.killers[thread.ply + 1][0] = NULL_MOVE;
    thread.killers[thread.ply + 1][1] = NULL_MOVE;
    PV child_pv;
    // child_pv.reserve(32);
    UndoData undo_data = UndoData(thread.board.king_attackers);
    Move best_move = NULL_MOVE, move = NULL_MOVE;
    // std::vector<Move> captures_tried, quiets_tried;
    Move captures_tried[64], quiets_tried[128];
    Move *captures_p = captures_tried, *quiets_p = quiets_tried;

    int score, best_score = -CHECKMATE, searched_moves = 0, extended_depth, reduction;
    int eval_score = tt_score != INF ? tt_score : evaluate(thread.board);

    // beta pruning
    if(!is_pv
    && !in_check
    && depth >= MIN_BETA_PRUNING_DEPTH
    && eval_score - BETA_MARGIN * depth > beta)
        return eval_score;

    // null move pruning
    if(!is_root
    && !is_pv
    && !in_check 
    && eval_score >= beta
    && depth >= MIN_NULL_MOVE_PRUNING_DEPTH
    // && *(thread.move_stack_p - 1) != NULL_MOVE
    && (tt_bound == -1 || tt_score >= beta || tt_bound != UPPER_BOUND)) {
        thread.board.make_move(NULL_MOVE, undo_data);
        // thread.move_stack.push_back(NULL_MOVE);
        // (thread.move_stack_p++) = NULL_MOVE;
        thread.ply++;

        int null_move_value = -search(thread, child_pv, -beta, -beta + 1, depth - 3);
        // assert(thread.move_stack.back() == NULL_MOVE);
        // thread.move_stack.pop_back();
        // (thread.move_stack_p--);
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

    NewMovePicker move_picker = NewMovePicker(thread, tt_move);
    // MovePicker move_picker = MovePicker(thread, tt_move);

    while(true) {
        move = move_picker.next_move();

        if(move == NULL_MOVE 
        || *thread.stop_search)
            break;
        
        if(!is_pv
        && get_flag(move) == QUIET_MOVE
        && !in_check
        && is_futile
        && searched_moves > 0)
            continue;

        // we have already checked the validity of the captures
        // if(!thread.board.fast_move_valid(move)) {
        if(!thread.board.new_fast_move_valid(move))
            continue;

        assert(thread.board.move_valid(move));
        assert(get_flag(move) != QUIET_MOVE || thread.board.color_at[get_to(move)] == EMPTY);
        assert(thread.board.color_at[get_from(move)] == thread.board.side);

        thread.board.new_make_move(move, undo_data);
        // thread.board.make_move(move, undo_data);
        // thread.move_stack.push_back(move);
        thread.ply++;

        extended_depth = depth + ((is_pv && in_check) ? 1 : 0);
        
        if(searched_moves > 3) { // taken from Halogen (https://github.com/KierenP/Halogen)
			reduction = LMR[std::min(63, std::max(0, depth))][std::min(63, std::max(0, searched_moves))];

            if(is_pv)
                reduction++;

			reduction = std::max(0, reduction);

            score = -search(thread, child_pv, -alpha - 1, -alpha, extended_depth - 1 - reduction);

			if(score <= alpha) {
                // thread.board.take_back(undo_data);
                thread.board.new_take_back(undo_data);
                // thread.move_stack.pop_back();
                thread.ply--;
				continue;
			}
        }

        if(get_flag(move) == QUIET_MOVE || get_flag(move) == CASTLING_MOVE) {
            // quiets_tried.push_back(move);
            *(quiets_p++) = move;
        } else {
            // captures_tried.push_back(move);
            *(captures_p++) = move;
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

        thread.board.new_take_back(undo_data);
        // thread.board.take_back(undo_data);
        // thread.move_stack.pop_back();
        thread.ply--;

        if(score > best_score) {
            best_score = score;
            best_move = move;

            if(score > alpha) {
                pv.clear();
                pv.push_back(move);
                pv.insert(pv.end(), child_pv.begin(), child_pv.end());

                if(is_root) {
                    printf("info depth %d time %d nodes %d cp score %d pv",
                           thread.depth, elapsed_time(), thread.nodes, thread.root_value);
                    for(int i = 0; i < (int)pv.size(); i++) {
                        printf(" %s", move_to_str(pv[i]).c_str());
                    }
                    printf("\n");
                }

                alpha = score;

                assert(!pv.empty());

                if(alpha >= beta) {
                    if(debug_mode)
                        cout << "Breaking from the search" << endl;
                    break;
                }
            }
        }
    }

    if(best_score == -CHECKMATE)
        return in_check ? -CHECKMATE + thread.ply : 0; // checkmate or stalemate

    if(best_score >= beta && get_flag(best_move) != CAPTURE_MOVE)
        update_quiet_history(thread, best_move, quiets_tried, quiets_p, depth);

    if(best_score >= beta)
        update_capture_history(thread, best_move, captures_tried, captures_p, depth); 

    // if(true) {
        // do nothing if we want to disable tt
    // } else
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

int q_search(Thread& thread, int alpha, int beta) {

    if((thread.nodes & 4095) == 0 && elapsed_time() >= max_search_time) {
        *thread.stop_search = true;
    }

    if(*thread.stop_search)
        return 0;

    if(thread.board.is_draw())
        return thread.nodes & 2;

    if(thread.ply >= MAX_PLY)
        return evaluate(thread.board);

    thread.nodes++;
    
    // pv.clear();
    Move tt_move = NULL_MOVE;
    int score, tt_score, tt_bound = -1;

    // it will return true if it causes a cutoff or is an exact value
    if(tt.retrieve(
        thread.board.key, tt_move,
        tt_score, tt_bound, alpha, beta, 0, thread.ply
    ))
       return tt_score; 

    // PV child_pv;
    UndoData undo_data = UndoData(thread.board.king_attackers);
    int best_score = tt_bound != -1 ? tt_score : evaluate(thread.board);
    bool in_check = bool(thread.board.king_attackers);

    // eval pruning
    alpha = std::max(alpha, best_score);
    if(alpha >= beta) {
        return alpha;
    }

    NewMovePicker move_picker = NewMovePicker(thread, tt_move, true);
    // MovePicker move_picker = MovePicker(thread, tt_move, true);

    while(true) {
        Move move = move_picker.next_move();

        if(move == NULL_MOVE)
            break;


        // if(!thread.board.fast_move_valid(move)) {
        if(!thread.board.new_fast_move_valid(move)) {
            assert(!thread.board.new_fast_move_valid(move));
            continue;
        }

        assert(!in_check || get_flag(move) != CASTLING_MOVE);

        thread.board.new_make_move(move, undo_data);
        // thread.board.make_move(move, undo_data);
        thread.ply++;

        score = -q_search(thread, -beta, -alpha);

        // thread.board.take_back(undo_data); 
        thread.board.new_take_back(undo_data); 
        thread.ply--;

        if(score > best_score) {
            best_score = score;

            if(best_score > alpha) {
                alpha = best_score;

                // pv.clear();
                // pv.push_back(move);
                // pv.insert(pv.end(), child_pv.begin(), child_pv.end());
                // assert(!pv.empty());

                if(alpha >= beta) {
                    return alpha;
                }
            }
        }
    }

    return best_score;
}

// void update_quiet_history(Thread& thread, const Move best_move, const std::vector<Move>& quiets_tried, const int depth) {
void update_quiet_history(Thread& thread, const Move best_move, Move* quiets_p, Move* quiets_end, const int depth) {
    // the best move is a quiet move
    if(thread.killers[thread.ply][0] != best_move) {
        thread.killers[thread.ply][1] = thread.killers[thread.ply][1];
        thread.killers[thread.ply][0] = best_move;
    }

    if((quiets_p - quiets_p) == 1 && depth < 4) {
        return;
    }

    const int bonus = std::min(depth * depth, MAX_HISTORY_BONUS);

    // for(int i = 0; i < quiets_tried.size(); i++) {
    for(; quiets_p < quiets_end; quiets_p++) {
        // int entry = thread.quiet_history[thread.board.side][get_from(*quiets_p)][get_to(*quiets_p)];    
        // entry += HISTORY_MULTIPLIER * (*quiets_p == best_move ? bonus : -bonus)
        //         - entry * bonus / HISTORY_DIVISOR;
        // entry += HISTORY_MULTIPLIER * delta - entry * std::abs(delta) / HISTORY_DIVISOR;
        thread.quiet_history[thread.board.side][get_from(*quiets_p)][get_to(*quiets_p)] 
            += HISTORY_MULTIPLIER * (*quiets_p == best_move ? bonus : -bonus)
            - thread.quiet_history[thread.board.side][get_from(*quiets_p)][get_to(*quiets_p)] * bonus / HISTORY_DIVISOR;
    }
}

// void update_capture_history(Thread& thread, const Move best_move, const std::vector<Move>& captures_tried, const int depth) {
void update_capture_history(Thread& thread, const Move best_move, Move* move_p, Move* captures_end, const int depth) {
    const int bonus = std::min(depth * depth, MAX_HISTORY_BONUS);
    uint8_t captured;

    // int delta, from, to, flag, piece, captured, entry;

    // for(int i = 0; i < captures_tried.size(); i++) {
    for(; move_p < captures_end; move_p++) {
        // delta = captures_tried[i] == best_move ? bonus : -bonus;
        // from = get_from(captures_tried[i]);
        // to = get_to(captures_tried[i]);
        // flag = get_flag(captures_tried[i]);
        // piece = thread.board.piece_at[from];
        captured = thread.board.piece_at[get_to(*move_p)];

        assert(get_flag(*move_p) != NULL_MOVE
            && get_flag(*move_p) != QUIET_MOVE
            && get_flag(*move_p) != CASTLING_MOVE);

        if(get_flag(*move_p) != CAPTURE_MOVE) // enpassant
            captured = PAWN;
        
        assert(captured >= PAWN && captured <= KING);

        thread.capture_history[thread.board.piece_at[get_from(*move_p)]][get_to(*move_p)][captured]
            += HISTORY_MULTIPLIER * (*move_p == best_move ? bonus : -bonus)
            -  thread.capture_history[thread.board.piece_at[get_from(*move_p)]][get_to(*move_p)][captured] * bonus / HISTORY_DIVISOR;
        
        assert(thread.capture_history[thread.board.piece_at[get_from(*move_p)]][get_to(*move_p)][captured] <=  100000000
            && thread.capture_history[thread.board.piece_at[get_from(*move_p)]][get_to(*move_p)][captured] >= -100000000);

        // entry = thread.capture_history[piece][to][captured];
        // entry += HISTORY_MULTIPLIER * delta - entry * std::abs(delta) - HISTORY_DIVISOR;
        // entry = std::max(-10000, std::min(10000, entry));
        // thread.capture_history[piece][to][captured] = entry;
    }
}

