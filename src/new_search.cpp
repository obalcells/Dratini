#include <vector>
#include "defs.h"
#include "engine.h"
#include "board.h"
#include "new_move_picker.h"
#include "new_search.h"
#include "sungorus_eval.h"

int new_search(NewThread& thread, int ply, int alpha, int beta, int depth, std::vector<Move>& pv);
int quiesce(NewThread& thread, int ply, int alpha, int beta, std::vector<Move> pv);

void new_think(Engine& engine) {
    cerr << "Initializing thread" << endl;
    NewThread thread = NewThread(engine.board);
    cerr << "Thread initialized" << endl;
    thread.clear_hist();
    tt.tt_date = (tt.tt_date + 1) & 255;
    thread.abort_search = false;
    thread.start_time = GetMS();

    for(thread.root_depth = 1; thread.root_depth <= 12; thread.root_depth++) {
        printf("info depth %d\n", thread.root_depth);
        int score = new_search(thread, 0, -INF, INF, thread.root_depth, thread.pv);
        engine.best_move = thread.pv[0];
        engine.ponder_move = thread.pv.size() > 1 ? thread.pv[1] : NULL_MOVE;
        if(thread.abort_search) {
            break;
        }
        engine.score = score;
    }
    engine.nodes = thread.nodes;
    engine.search_time = GetMS() - thread.start_time;
}

int new_search(NewThread& thread, int ply, int alpha, int beta, int depth, std::vector<Move>& pv) {
    int best, score, new_depth, bound;
    Move move;
    std::vector<Move> new_pv;
    UndoData undo_data;
    if(depth <= 0) {
        int quiesce_score =  quiesce(thread, ply, alpha, beta, pv);
        return quiesce_score;
    }
    thread.nodes++;
    thread.check_time();
    if(thread.abort_search) return 0; 
    if(ply) pv.clear();
    if(thread.board.is_draw() && ply) return 0;
    move = NULL_MOVE;
    if(ply && tt.retrieve(thread.board.key, move, score, bound, alpha, beta, depth, ply))
        return score;
    if(ply >= 31)
        return evaluate(thread.board);
    if(depth > 1 && beta <= evaluate(thread.board) && !bool(thread.board.king_attackers)
    && (thread.board.occ_mask & ~(thread.board.bits[WHITE_KING] | thread.board.bits[BLACK_KING] 
        | thread.board.bits[WHITE_PAWN] | thread.board.bits[BLACK_PAWN])) != 0) {
        undo_data = thread.board.make_move(NULL_MOVE);
        score = -new_search(thread, ply + 1, -beta, -beta + 1, depth - 3, new_pv);
        thread.board.take_back(undo_data);
        if(thread.abort_search) return 0;
        if(score >= beta) { 
            tt.save(thread.board.key, NULL_MOVE, score, LOWER_BOUND, depth, ply);
            return score;
        }
    }
    best = -INF;
    NewSearchMovePicker move_picker = NewSearchMovePicker(thread, move, ply);
    while((move = move_picker.next_move())) {
        if(!thread.board.fast_move_valid(move))
            continue;
        new_depth = depth - 1 + bool(thread.board.king_attackers);
        undo_data = thread.board.make_move(move);
        if (best == -INF)
            score = -new_search(thread, ply + 1, -beta, -alpha, new_depth, new_pv);
        else {
            score = -new_search(thread, ply + 1, -alpha - 1, -alpha, new_depth, new_pv);
            if (!thread.abort_search && score > alpha && score < beta)
                score = -new_search(thread, ply + 1, -beta, -alpha, new_depth, new_pv);
        }
        thread.board.take_back(undo_data);
        if(thread.abort_search) return 0;
        if(score >= beta) {
            thread.hist(move, depth, ply);
            tt.save(thread.board.key, move, score, LOWER_BOUND, depth, ply);
            return score;
        }
        if(score > best) {
            best = score;
            if(score > alpha) {
                alpha = score;
                pv.clear(); 
                pv.push_back(move);
                if(!new_pv.empty())
                    pv.insert(pv.end(), new_pv.begin(), new_pv.end());
                if(!ply) {
                    printf("info depth %d time %d nodes %d cp score %d pv",
                           thread.root_depth, GetMS() - thread.start_time, thread.nodes, score);
                    for(int i = 0; i < (int)pv.size(); i++) {
                        printf(" %s", move_to_str(pv[i]).c_str());
                    }
                    printf("\n");
                }
            }
        }
    }
    if(best == -INF)
        return thread.board.king_attackers ? (-CHECKMATE + ply) : 0;
    if(!pv.empty()) {
        thread.hist(pv[0], depth, ply);
        tt.save(thread.board.key, pv[0], best, EXACT_BOUND, depth, ply);
    } else
        tt.save(thread.board.key, NULL_MOVE, best, UPPER_BOUND, depth, ply);
    return best;
}

int quiesce(NewThread& thread, int ply, int alpha, int beta, std::vector<Move> pv) {
    int best, score;
    Move move;
    std::vector<Move> new_pv;
    UndoData undo_data;

    thread.nodes++;
    thread.check_time();
    if(thread.abort_search) return 0;
    pv.clear();
    if(thread.board.is_draw()) return 0;
    if(ply >= 31)
        return evaluate(thread.board);
    best = evaluate(thread.board);
    if(best >= beta)
        return best;
    if(best > alpha)
        alpha = best;
    NewSearchMovePicker move_picker = NewSearchMovePicker(thread, NULL_MOVE, ply, true);
    while((move = move_picker.next_move())) {
        if(!thread.board.fast_move_valid(move))
            continue;
        undo_data = thread.board.make_move(move);
        score = -quiesce(thread, ply + 1, -beta, -alpha, new_pv);
        thread.board.take_back(undo_data);
        if(thread.abort_search) return 0;
        if(score >= beta)
            return beta;
        if(score > best) {
            best = score;
            if(score > alpha) {
                alpha = score;        
                pv.clear();
                pv.push_back(move);
                pv.insert(pv.end(), new_pv.begin(), new_pv.end());
            }
        }
    }
    return best;
}
