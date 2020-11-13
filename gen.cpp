#include <vector>
#include <iostream>
#include <cassert>
#include "defs.h"
#include "protos.h"
#include "data.h"
#include "move.h"
#include "board.h"
#include "state.h"
#include "stats.h"

void add_move(char from, char to) {
 	assert(from >= 0 && to >= 0);
	State state_before = State();
 	Move m = make_move(from, to, QUEEN);
  	if(!in_check(xside)) {
		take_back(m);
		int error_code = move_valid(from, to); // this should be disabled for production
		if(error_code != 0) {
			print_board();
			assert(error_code == 0);
		}
		int score;
		score += history[side][from][to];
		if(piece[to] != EMPTY) {
			score = 10000 + piece_value[piece[to]] - piece_value[piece[from]];
		}
		unordered_move_stack.push_back(std::make_pair(score, m));
	} else {
		take_back(m);
  	}
	State state_after = State();
	if(!state_before.same(state_after)) {
		std::cout << "State is different after taking back." << endl;
		std::cout << "Move is " << str_move(from, to) << endl << endl;
		std::cout << m.captured << endl; 
		std::cout << "State before: " << endl;
		state_before.print();
		std::cout << "State now is: " << endl;
		state_after.print();
		std::cout << "Move stack:" << endl << endl;
		for(Move move : taken_moves) {
			std::cout << str_move(move.from, move.to) << " ";
		}
		std::cout << endl << endl;
		std::cout << "Saving state with name: error-" + str_move(from, to) << endl << endl;
		state_before.set();
		save_snapshot("error-" + str_move(from, to));
		assert(state_before.same(state_after));
	} 
}

void order_and_push() {
	sort(unordered_move_stack.rbegin(), unordered_move_stack.rend());
	while(!unordered_move_stack.empty()) {
		move_stack.push_back(unordered_move_stack.back().second);
		unordered_move_stack.pop_back();
	}
}

void generate_capture_moves() {
	// stats::activate(GENERATE_CAPTURES);

	for (char pos = 0; pos < 64; pos++)
		if (color[pos] == side) {
			if (piece[pos] == PAWN) {
				// diagonal-capture
				char one_forward = (side == WHITE ? pos + 8 : pos - 8);

				if (valid_distance(pos, one_forward - 1) && color[one_forward - 1] == xside) {
					add_move(pos, one_forward - 1);
				}
				if (valid_distance(pos, one_forward + 1) && color[one_forward + 1] == xside) {
					add_move(pos, one_forward + 1);
				}
				// enpassant
				if ((side == WHITE && row(pos) == 4) || (side == BLACK && row(pos) == 3)) {
					// to the left
					if (col(pos) > 0 && color[pos - 1] == xside && piece[pos - 1] == PAWN && enpassant == col(pos - 1)) {
						if (side == WHITE) {
							add_move(pos, pos + 7);
						} else {
							add_move(pos, pos - 9);
						}
					}
					// to the right
					else if (col(pos) < 7 && color[pos + 1] == xside && piece[pos + 1] == PAWN && enpassant == col(pos + 1)) {
						if (side == WHITE) {
							add_move(pos, pos + 9);
						} else {
							add_move(pos, pos - 7);
						}
					}
				}
			} else {
				for (int i = 0; i < 8; i++) {
					if (offset[piece[pos]][i] == 0) break;
					char new_pos = pos + offset[piece[pos]][i];

					if (!slide[piece[pos]])
					{
						if (valid_distance(pos, new_pos) && color[new_pos] == xside)
						{
							add_move(pos, new_pos);
						}
					}
					else
					{
						char prev_pos = pos;
						while (valid_distance(prev_pos, new_pos) && color[new_pos] != side)
						{
							if (color[new_pos] == xside)
							{
								add_move(pos, new_pos);
								break;
							}
							prev_pos = new_pos;
							new_pos += offset[piece[pos]][i];
						}
					}
				}
			}
		}
	order_and_push();
}

void generate_moves() {
	for(int pos = 0; pos < 64; pos++) if(color[pos] == side) {
		if(piece[pos] == PAWN) {
			// one forward
			char one_forward = (side == WHITE ? pos + 8 : pos - 8);
			int captured = EMPTY;
			if(row(one_forward) == 0 || row(one_forward) == 7) captured = PAWN;
			if(color[one_forward] == EMPTY) add_move(pos, one_forward);
			// two forward
			if((side == WHITE && row(pos) == 1) || (side == BLACK && row(pos) == 7)) {
				char two_forward = (side == WHITE ? pos + 16 : pos - 16);
				if(color[one_forward] == EMPTY && color[two_forward] == EMPTY) {
					add_move(pos, two_forward);
				}
			}
			// eating
			if(valid_distance(pos, one_forward - 1) && color[one_forward - 1] == xside) {
				add_move(pos, one_forward - 1);
			}
			if(valid_distance(pos, one_forward + 1) && color[one_forward + 1] == xside) {
				add_move(pos, one_forward + 1);
			}
			// enpassant
			if((side == WHITE && row(pos) == 4) || (side == BLACK && row(pos) == 3)) {
				// to the left
				if(col(pos) > 0 && color[pos - 1] == xside && piece[pos - 1] == PAWN && enpassant == col(pos - 1)) {
					if(side == WHITE) add_move(pos, pos + 7);
					else add_move(pos, pos - 9);
				// to the right
				} else if(col(pos) < 7 && color[pos + 1] == xside && piece[pos + 1] == PAWN && enpassant == col(pos + 1)) {
					if(side == WHITE) add_move(pos, pos + 9);
					else add_move(pos, pos - 7);
				}
			}
		} else {
			if(piece[pos] == KING) {
				// castling
				if(side == WHITE && (castling & 4)) {
					if((castling & 1) && move_valid(4, 2) == 0) add_move(4, 2);
					if((castling & 2) && move_valid(4, 6) == 0) add_move(4, 6);
				} else if(side == BLACK && castling & 32) {
					if((castling & 8) && move_valid(60, 58) == 0) add_move(60, 58);
					if((castling & 16) && move_valid(60, 62) == 0) add_move(60, 62);
				}
			}

			for(int i = 0; i < 8; i++) {
				if(offset[piece[pos]][i] == 0) break;
        		int new_pos = pos + offset[piece[pos]][i];

				if(!slide[piece[pos]]) {
					if(valid_distance(pos, new_pos) && color[new_pos] != side) {
						add_move(pos, new_pos);
					}
				} else {
					int prev_pos = pos;
					while(valid_distance(prev_pos, new_pos) && color[new_pos] != side) {
						add_move(pos, new_pos);
						if(color[new_pos] == xside) break;
						prev_pos = new_pos;
						new_pos += offset[piece[pos]][i];
					}
				}
			}
		}
	}
	order_and_push();
}