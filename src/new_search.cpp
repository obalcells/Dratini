
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include "defs.h"
#include "engine.h"
#include "board.h"
#include "new_move_picker.h"
#include "sungorus_eval.h"


int GetMS(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

struct NewThread {
    int history[12][64];
    Move killer[32][2];
    std::vector<Move> pv;
    int nodes, start_time, move_time, root_depth;
    bool abort_search;
    Board board;

    NewThread(Board& _board) {
        board = _board;
        move_time = 8000;

        for(int i = 0; i < 32; i++) {
            pv[i] = NULL_MOVE;
        }
    }

    void clear_hist() {
        int i, j;
        for (i = 0; i < 12; i++)
            for (j = 0; j < 64; j++)
                history[i][j] = 0;
        for (i = 0; i < 32; i++) {
            killer[i][0] = 0;
            killer[i][1] = 0;
        }
    }

    void check_time() {
        if (move_time >= 0 && GetMS() - start_time >= move_time)
            abort_search = 1;
    }

    void hist(Move move, int depth, int ply) {
        if(board.piece_at[get_to(move)] != EMPTY || get_flag(move) >= 5 || get_flag(move) == ENPASSANT_MOVE)
            return;

        history[get_from(move)][get_to(move)] += depth;
        if(move != killer[ply][0]) {
            killer[ply][1] = killer[ply][0];
            killer[ply][0] = move;
        }
    }
};

int new_search(NewThread& thread, int ply, int alpha, int beta, int depth, std::vector<Move>& pv);
int quiesce(NewThread& thread, int ply, int alpha, int beta, std::vector<Move> pv);

void new_think(Engine& engine) {
    NewThread thread = NewThread(engine.board);
    thread.clear_hist();
    tt.tt_date = (tt.tt_date + 1) & 255;
    thread.abort_search = false;
    thread.start_time = GetMS();

    for(thread.root_depth = 1; thread.root_depth < 256; thread.root_depth++) {
        printf("info depth %d\n", thread.root_depth);
        new_search(thread, 0, -INF, INF, thread.root_depth, thread.pv);
        if(thread.abort_search) {
            break;
        }
    }
}

int new_search(NewThread& thread, int ply, int alpha, int beta, int depth, std::vector<Move>& pv) {
    int best, score, new_depth, bound;
    Move move;
    std::vector<Move> new_pv;
    UndoData undo_data;
    if(depth <= 0) {
        return quiesce(thread, ply, alpha, beta, pv);
    }
    thread.nodes++;
    thread.check_time();
    if(thread.abort_search) return 0; 
    if(ply) pv.clear();
    if(thread.board.is_draw() && ply) return 0;
    move = NULL_MOVE;
    if(tt.retrieve(thread.board.key, move, score, bound, alpha, beta, depth, ply))
        return score;
    if(ply >= 31)
        return evaluate(thread.board);
    if(depth > 1 && beta <= evaluate(thread.board) && !thread.board.in_check()
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
    NewMovePicker move_picker = NewMovePicker(thread, move);
    while((move = move_picker.next_move())) {
        if(!thread.board.fast_move_valid(move))
            continue;
        new_depth = depth - 1 + thread.board.in_check();
        undo_data = thread.board.make_move(move);
        if (best == -INF)
            score = -new_search(thread, ply + 1, -beta, -alpha, new_depth, new_pv);
        else {
            score = -new_search(thread, ply + 1, -alpha - 1, -alpha, new_depth, new_pv);
            if (!thread.abort_search && score > alpha && score < beta)
                score = -new_search(p, ply + 1, -beta, -alpha, new_depth, new_pv);
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
                pv.insert(pv.end(), new_pv.begin(), new_pv.end());
                if(!ply) {
                    printf("info depth %d time %d nodes %d cp score %d pv",
                           thread.root_depth, elapsed_time(), thread.nodes, score);
                    for(int i = 0; i < (int)pv.size(); i++) {
                        printf(" %s", move_to_str(pv[i]).c_str());
                    }
                    printf("\n");
                }
            }
        }
    }
    if(best == -INF)
        return board.in_check() ? (-CHECKMATE + ply) : 0;
    if(!pv.empty()) {
        thread.hist(pv[0], depth, ply);
        tt.save(thread.board.key, pv[0], best, EXACT_BOUND, depth, ply);
    } else
        tt.save(thread.board.key, NULL_MOVE, best, UPPER, depth, ply);
    return best;
}

int quiesce(Thread& thread, int ply, int alpha, int beta, std::vector<Move> pv) {
    int best, score;
    Move move;
    std::vector<Move> new_pv;
    UndoData undo_data;

    thread.nodes++;
    thread.check_time():
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
    NewMovePicker move_picker = NewMovePicker(thread, NULL_MOVE, true);
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
