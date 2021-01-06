#include <vector>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <random>
#include "defs.h"
#include "data.h"
#include "board.h"
#include "stats.h"

namespace {
    std::mt19937 g(std::random_device {}());
}

void add_move(Position& position, Move move) {
    stats.change_phase(CHECK);
	if(position.move_valid(move)) {
		Position prev_position = position; // this is extremely slow
    	position.make_move(move);
    	if (position.in_check(position.xside)) {
			throw("Position is invalid after it has been marked as valid by move_valid checker");
		}
		position = prev_position;
        int score = history[position.side][from(move)][to(move)];
        if (position.piece[to(move)] != EMPTY) {
            score = 10000 + piece_value[position.piece[to(move)]] - piece_value[position.piece[from(move)]];
        }
        unordered_move_stack.push_back(std::make_pair(score, move));
    }
	stats.revert_phase();
}

void order_and_push() {
    stats.change_phase(MOVE_ORD);
#ifndef SELF_PLAY
    sort(unordered_move_stack.rbegin(), unordered_move_stack.rend());
#else
    std::shuffle(unordered_move_stack.begin(), unordered_move_stack.end(), g);
#endif
    while(!unordered_move_stack.empty()) {
        move_stack.push_back(unordered_move_stack.back().second);
        unordered_move_stack.pop_back();
    }
}

void generate_capture_moves(Position& position) {
    stats.change_phase(CAP_MOVE_GEN);
    for (char pos = 0; pos < 64; pos++) {
        if (position.color[pos] == position.side) {
            if (position.piece[pos] == PAWN) {
                // diagonal-capture
                char one_forward = (position.side == WHITE ? pos + 8 : pos - 8);
                if (valid_distance(pos, one_forward - 1) &&
                    position.color[one_forward - 1] == position.xside)
                    add_move(position, Move(pos, one_forward - 1));
                if (valid_distance(pos, one_forward + 1) &&
                    position.color[one_forward + 1] == position.xside)
                    add_move(position, Move(pos, one_forward + 1));
                // enpassant
                if ((position.side == WHITE && row(pos) == 4) ||
                    (position.side == BLACK && row(pos) == 3)) {
                    // to the left
                    if (col(pos) > 0 && position.color[pos - 1] == position.xside &&
                        position.piece[pos - 1] == PAWN &&
                        position.enpassant == col(pos - 1)) {
                        if (position.side == WHITE)
                            add_move(position, Move(pos, pos + 7));
                        else
                            add_move(position, Move(pos, pos - 9));
                    }
                    // to the right
                    else if (col(pos) < 7 && position.color[pos + 1] == position.xside &&
                        position.piece[pos + 1] == PAWN &&
                        position.enpassant == col(pos + 1)) {
                        if (position.side == WHITE)
                            add_move(position, Move(pos, pos + 9));
                        else
                            add_move(position, Move(pos, pos - 7));
                    }
                }
                continue;
            }

            const int p = position.piece[pos];

            for (int i = 0; offset[p][i]; i++) {
                int delta = offset[p][i];

                if (!slide[p]) {
                    assert(p == KNIGHT || p == KING);
                    if (valid_distance(pos, pos + delta) &&
                        position.color[pos + delta] != position.side) {
                        add_move(position, Move(pos, pos + delta));
                    }
                } else {
                    assert(p == BISHOP || p == ROOK || p == QUEEN);
                    int new_pos = pos;
                    while (valid_pos(new_pos + delta) &&
                        distance(new_pos, new_pos + delta) <= 2 &&
                        position.color[new_pos + delta] != position.side) {
                        add_move(position, Move(new_pos, new_pos + delta));
                        if (position.color[new_pos] == position.xside) break; // we eat
                        new_pos = new_pos + delta;
                    }
                }
            }
        }
	}
	order_and_push();
}

void generate_moves(Position & position) {
    stats.change_phase(MOVE_GEN);
    /* Brute-force move generation for testing bitboard-based generation */
    for(int from_sq = 0; from_sq < 64; from_sq++) if(position.color[from_sq] == position.side) { 
        for(int to_sq = 0; to_sq < 64; to_sq++) {
            if(position.move_valid(Move(from_sq, to_sq))) { 
                move_stack.push_back(Move(from_sq, to_sq));
                // add_move(position, Move(from_sq, to_sq));
            }
        }
    }
    // order_and_push();
    return;

    for (int pos = 0; pos < 64; pos++) {
        if (position.color[pos] == position.side) {
            if (position.piece[pos] == PAWN) {
                // one forward
                char one_forward = (position.side == WHITE ? pos + 8 : pos - 8);
                int captured = EMPTY;
                if (row(one_forward) == 0 || row(one_forward) == 7) captured = PAWN;
                if (position.color[one_forward] == EMPTY)
                    add_move(position, Move(pos, one_forward));
                // two forward
                if ((position.side == WHITE && row(pos) == 1) ||
                    (position.side == BLACK && row(pos) == 7)) {
                    char two_forward = (position.side == WHITE ? pos + 16 : pos - 16);
                    if (position.color[one_forward] == EMPTY &&
                        position.color[two_forward] == EMPTY) {
                        add_move(position, Move(pos, two_forward));
                    }
                }
                // eating
                if (valid_distance(pos, one_forward - 1) &&
                    position.color[one_forward - 1] == position.xside) {
                    add_move(position, Move(pos, one_forward - 1));
                }
                if (valid_distance(pos, one_forward + 1) &&
                    position.color[one_forward + 1] == position.xside) {
                    add_move(position, Move(pos, one_forward + 1));
                }
                // enpassant
                if ((position.side == WHITE && row(pos) == 4) ||
                    (position.side == BLACK && row(pos) == 3)) {
                    // to the left
                    if (col(pos) > 0 && position.color[pos - 1] == position.xside &&
                        position.piece[pos - 1] == PAWN &&
                        position.enpassant == col(pos - 1)) {
                        if (position.side == WHITE)
                            add_move(position, Move(pos, pos + 7));
                        else
                            add_move(position, Move(pos, pos - 9));
                        // to the right
                    } else if (col(pos) < 7 &&
                        position.color[pos + 1] == position.xside &&
                        position.piece[pos + 1] == PAWN &&
                        position.enpassant == col(pos + 1)) {
                        if (position.side == WHITE)
                            add_move(position, Move(pos, pos + 9));
                        else
                            add_move(position, Move(pos, pos - 7));
                    }
                }
                continue;
            } else {
                if (position.piece[pos] == KING) {
                    // castling
                    if (position.side == WHITE && (position.castling & 4)) {
                        if ((position.castling & 1) && position.move_valid(Move(4, 2)))
                            add_move(position, Move(4, 2));
                        if ((position.castling & 2) && position.move_valid(Move(4, 6)))
                            add_move(position, Move(4, 6));
                    } else if (position.side == BLACK && position.castling & 32) {
                        if ((position.castling & 8) && position.move_valid(Move(60, 58)))
                            add_move(position, Move(60, 58));
                        if ((position.castling & 16) && position.move_valid(Move(60, 62)))
                            add_move(position, Move(60, 62));
                    }
                }

                const int p = position.piece[pos];

                for (int i = 0; offset[p][i]; i++) {
                    int delta = offset[p][i];

                    if (!slide[p]) {
                        assert(p == KNIGHT || p == KING);
                        if (valid_distance(pos, pos + delta) &&
                            position.color[pos + delta] != position.side) {
                            add_move(position, Move(pos, pos + delta));
                        }
                    } else {
                        assert(p == BISHOP || p == ROOK || p == QUEEN);
                        int new_pos = pos;
                        while (valid_pos(new_pos + delta) &&
                            distance(new_pos, new_pos + delta) <= 2 &&
                            position.color[new_pos + delta] != position.side) {
                            add_move(position, Move(new_pos, new_pos + delta));
                            if (position.color[new_pos] == position.xside) break; // we eat
                            new_pos = new_pos + delta;
                        }
                    }
                }
            }
        }
	}
    order_and_push();
}