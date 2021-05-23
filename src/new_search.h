#include <unistd.h>
#include <cassert>
#include <sys/time.h>
#include "defs.h"
#include "engine.h"
#include "magicmoves.h"
#include "bitboard.h"
#include "board.h"
#include "gen.h"
#include "search.h"

#define get_side_mask(_side) (_side == WHITE ? \
    (board->bits[WHITE_PAWN] | board->bits[WHITE_KNIGHT] | board->bits[WHITE_BISHOP] | board->bits[WHITE_ROOK] | board->bits[WHITE_QUEEN] | board->bits[WHITE_KING]) : \
    (board->bits[BLACK_PAWN] | board->bits[BLACK_KNIGHT] | board->bits[BLACK_BISHOP] | board->bits[BLACK_ROOK] | board->bits[BLACK_QUEEN] | board->bits[BLACK_KING]))
#define get_piece_mask(piece) board->bits[piece]
#define get_all_mask() board->occ_mask
#define get_pawn_mask(_side) (_side == WHITE ? board->bits[WHITE_PAWN] : board->bits[BLACK_PAWN])
#define get_knight_mask(_side) (_side == WHITE ? board->bits[WHITE_KNIGHT] : board->bits[BLACK_KNIGHT])
#define get_bishop_mask(_side) (_side == WHITE ? board->bits[WHITE_BISHOP] : board->bits[BLACK_BISHOP])
#define get_rook_mask(_side) (_side == WHITE ? board->bits[WHITE_ROOK] : board->bits[BLACK_ROOK])
#define get_queen_mask(_side) (_side == WHITE ? board->bits[WHITE_QUEEN] : board->bits[BLACK_QUEEN])
#define get_king_mask(_side) (_side == WHITE ? board->bits[WHITE_KING] : board->bits[BLACK_KING])
#define get_piece(sq) (board->piece_at[sq] + (board->color_at[sq] == BLACK ? 6 : 0))
#define get_color(sq) (board->color_at[sq])
#define in_check() bool(board->king_attackers)

static inline bool move_is_straight(const Move move) {
    return row(get_from(move)) == row(get_to(move)) || col(get_from(move)) == col(get_to(move));
}

static inline bool move_is_diagonal(const Move move) {
    return abs(row(get_from(move)) - row(get_to(move))) == abs(col(get_from(move)) - col(get_to(move)));
}

static int GetMS(void) {
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
        move_time = 5000;
        nodes = 0;
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
        history[board.piece_at[get_from(move)] + (board.color_at[get_from(move)] == WHITE ? 0 : 6)][get_to(move)] += depth;
        if(move != killer[ply][0]) {
            killer[ply][1] = killer[ply][0];
            killer[ply][0] = move;
        }
    }
};

struct NewSearchMovePicker {
    NewThread *thread;
    Board* board;
    Move tt_move, killer_1, killer_2;
    std::vector<Move> moves, bad_captures;
    std::vector<int> scores;
    int next, badp, phase;
    bool quiesce;

    NewSearchMovePicker(NewThread& _thread, Move _tt_move, int ply, bool _quiesce = false) {
        thread = &_thread;
        board = &_thread.board;
        tt_move = _tt_move;
        killer_1 = _thread.killer[ply][0];
        killer_2 = _thread.killer[ply][1];
        quiesce = _quiesce;
        badp = 0;
        phase = 0;
    }

    Move next_move() {
        Move move;

        switch(phase) {
            case 0:
                move = tt_move;
                if(move != NULL_MOVE && board->move_valid(move)) {
                    phase = 1;
                    return move;
                }
            case 1:
                moves.reserve(20);
                generate_captures(moves, board);
                score_captures();
                next = 0; 
                phase = 2;
            case 2:
                while(next < moves.size()) {
                    move = select_best();
                    if(move == tt_move) {
                        continue;
                    }
                    if(bad_capture(move)) {
                        bad_captures.push_back(move);
                        continue;
                    }
                    return move;
                }
                assert(next == moves.size());
                if(quiesce) {
                    phase = 8;
                    return NULL_MOVE;
                }
            case 3:
                move = killer_1;
                if(move != NULL_MOVE 
                && move != tt_move
                && get_flag(move) != CAPTURE_MOVE
                && board->move_valid(move)) {
                    phase = 4;
                    return move;
                } 
            case 4:
                move = killer_2;
                if(move != NULL_MOVE
                && move != tt_move
                && get_flag(move) != CAPTURE_MOVE
                && board->move_valid(move)) {
                    phase = 5;
                    return move;
                }
            case 5:
                moves.reserve(20);
                assert(next == moves.size());
                generate_quiet(moves, board);
                score_quiet(next);
                phase = 6;
            case 6:
                while(next < moves.size()) {
                    move = select_best();
                    if(move == tt_move
                    || move == killer_1
                    || move == killer_2) {
                        continue;
                    }
                    return move;
                } 
                phase = 7;
            case 7: // bad captures
                while(badp < (int)bad_captures.size()) {
                    return bad_captures[badp++];
                }
                break;
        }
        return NULL_MOVE;
    }

    Move select_best() {
        for(int i = moves.size() - 1; i > next; i--) {
            if(scores[i] > scores[i - 1]) {
                std::swap(scores[i - 1], scores[i]);
                std::swap(moves[i - 1], moves[i]);
            }
        }
        return moves[next++];
    }

    bool bad_capture(Move move) {
        assert(get_flag(move) != QUIET_MOVE); 
        if(get_flag(move) != CAPTURE_MOVE) {
            return false; 
        }
        if(piece_value[board->piece_at[get_from(move)]] >= piece_value[board->piece_at[get_to(move)]]) {
            return false; 
        }
        return see(move) < 0; // static exchange eval
    }

    void score_captures() {
        for(int i = 0; i < moves.size(); i++) {
            // scores.push_back(thread->capture_history[board->piece_at[get_from(moves[i])]][get_to(moves[i])][board->piece_at[get_to(moves[i])]]);
            scores.push_back(mvvlva(moves[i]));
        }
        assert(scores.size() == moves.size());
    }

    void score_quiet(int quiet_start) {
        for(int i = quiet_start; i < moves.size(); i++) {
            scores.push_back(
                thread->history[board->piece_at[get_from(moves[i])] + (board->color_at[get_from(moves[i])] == WHITE ? 0 : 6)][get_to(moves[i])]
            );
        }
        assert(scores.size() == moves.size());
    }

    // Static Exchange Evaluation
    int see(Move move) const {
        const int to_sq = get_to(move);
        int from_sq = get_from(move), depth = 0, side = board->side;	
        int piece_at_to = board->piece_at[to_sq];
        uint64_t attacker_mask = get_attackers(to_sq, board->side, board) | get_attackers(to_sq, board->xside, board);	
        std::vector<int> score(16, 0);
        score[0] = 0; // we want to force the capture at root

        while(from_sq != -1) {
            score[depth + 1] = piece_value[piece_at_to] - score[depth];
            depth++;
            piece_at_to = board->piece_at[from_sq];
            attacker_mask ^= mask_sq(from_sq);
            side = !side;

            // check xrays
            if(move_is_diagonal(move)) {
                uint64_t _diagonal_mask;
                if(from_sq > to_sq) { // north
                    if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][NORTHEAST];
                    else _diagonal_mask = diagonal_mask[from_sq][NORTHWEST];
                } else { // south
                    if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][SOUTHEAST];
                    else _diagonal_mask = diagonal_mask[from_sq][SOUTHWEST];
                }

                attacker_mask |= Bmagic(from_sq, board->occ_mask) & _diagonal_mask
                & (get_queen_mask(side) | get_bishop_mask(side) | get_queen_mask(!side) | get_bishop_mask(!side)); 
            } else if(move_is_straight(move)) {
                if(row(from_sq) == row(to_sq)) {
                    attacker_mask |= Rmagic(from_sq, get_all_mask()) & straight_mask[from_sq][from_sq < to_sq ? EAST : WEST]
                    & (get_queen_mask(side) | get_rook_mask(side) | get_queen_mask(!side) | get_rook_mask(!side));
                } else {
                    attacker_mask |= Rmagic(from_sq, get_all_mask()) & straight_mask[from_sq][from_sq < to_sq ? SOUTH : NORTH]
                    & (get_queen_mask(side) | get_rook_mask(side) | get_queen_mask(!side) | get_rook_mask(!side));
                }
            }

            // if next_lva returns -1 it means that there are no more attackers from current attacker side 
            from_sq = next_lva(attacker_mask, side);
            // the king has put himself in check
            if(piece_at_to == WHITE_KING || piece_at_to == BLACK_KING) {
                depth--;
                from_sq = -1;
            }
        }

        score[0] = 10000; // we want to force the capture at the root
        score[depth + 1] = -score[depth];

        while(depth > 0) {
            // (Kind of) minimax optimization. At each node you can choose between not making the capture
            // or making the capture and assuming that the other side will play optimally after that
            score[depth] = std::max(-score[depth - 1], -score[depth + 1]);
            depth--;
        }

        return score[1];
    }

    int next_lva(const uint64_t& attacker_mask, const bool attacker_side) const {
        for(int piece = (attacker_side == WHITE ? WHITE_PAWN : BLACK_PAWN); piece < (attacker_side == WHITE ? BLACK_PAWN : 12); piece++) {
            if(attacker_mask & get_piece_mask(piece)) {
                return lsb(attacker_mask & get_piece_mask(piece));
            }
        } 
        return -1;
    }

    int mvvlva(const Move move) {
        if(board->piece_at[get_to(move)] != EMPTY)
            return board->piece_at[get_to(move)] * 5 - board->piece_at[get_from(move)] + 5;
        else if(board->piece_at[get_from(move)] == PAWN)
            return (get_flag(move) - KNIGHT_PROMOTION + KNIGHT) * 8;
        return 0;
    }
};

void new_think(Engine& engine);