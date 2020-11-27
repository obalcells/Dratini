#include <vector>
#include <iostream>
#include <cassert>
#include "defs.h"
#include "data.h"
#include "board.h"
#include "stats.h"

void add_move(Position& position, char from, char to) {
 	assert(from >= 0 && to >= 0);
	stats.change_phase(CHECK);
 	Move m = position.make_move(from, to, QUEEN);
  	if(!position.in_check(position.xside)) {
		position.take_back(m);
		int error_code = position.move_valid(from, to); // this should be disabled for production
		if(error_code != 0) {
			position.print_board();
			std::cerr << "Error code is " << error_code << '\n';
			assert(error_code == 0);
		}
		int score;
		score += history[position.side][from][to];
		if(position.piece[to] != EMPTY) {
			score = 10000 + piece_value[position.piece[to]] - piece_value[position.piece[from]];
		}
		unordered_move_stack.push_back(std::make_pair(score, m));
	} else {
		position.take_back(m);
  	}
	/*
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
	*/
	stats.revert_phase();
}

void order_and_push() {
	stats.change_phase(MOVE_ORD);
	sort(unordered_move_stack.rbegin(), unordered_move_stack.rend());
	while(!unordered_move_stack.empty()) {
		move_stack.push_back(unordered_move_stack.back().second);
		unordered_move_stack.pop_back();
	}
}

void generate_capture_moves(Position& position) {
	stats.change_phase(CAP_MOVE_GEN);
	for (char pos = 0; pos < 64; pos++)
		if (position.color[pos] == position.side) {
			if (position.piece[pos] == PAWN) {
				// diagonal-capture
				char one_forward = (position.side == WHITE ? pos + 8 : pos - 8);
				if (valid_distance(pos, one_forward - 1) && position.color[one_forward - 1] == position.xside)
					add_move(position, pos, one_forward - 1);
				if (valid_distance(pos, one_forward + 1) && position.color[one_forward + 1] == position.xside)
					add_move(position, pos, one_forward + 1);
				// enpassant
				if ((position.side == WHITE && row(pos) == 4) || (position.side == BLACK && row(pos) == 3)) {
					// to the left
					if (col(pos) > 0 && position.color[pos - 1] == position.xside && position.piece[pos - 1] == PAWN && position.enpassant == col(pos - 1)) {
						if (position.side == WHITE)
							add_move(position, pos, pos + 7);
						else 
							add_move(position, pos, pos - 9);
					}
					// to the right
					else if (col(pos) < 7 && position.color[pos + 1] == position.xside && position.piece[pos + 1] == PAWN && position.enpassant == col(pos + 1)) {
						if (position.side == WHITE)
							add_move(position, pos, pos + 9);
						else
							add_move(position, pos, pos - 7);
					}
				}
			} else {
				for(int i = 0; i < 8; i++) {
					if(offset[position.piece[pos]][i] == 0) break;
					char new_pos = pos + offset[position.piece[pos]][i];
					if (!slide[position.piece[pos]]) {
						if (valid_distance(pos, new_pos) && position.color[new_pos] == position.xside) {
							add_move(position, pos, new_pos);
						}
					} else {
						char prev_pos = pos;
						while (valid_distance(prev_pos, new_pos) && position.color[new_pos] != position.side) {
							if (position.color[new_pos] == position.xside) {
								add_move(position, pos, new_pos);
								break;
							}
							prev_pos = new_pos;
							new_pos += offset[position.piece[pos]][i];
						}
					}
				}
			}
		}
	order_and_push();
}

void generate_moves(Position& position) {
	stats.change_phase(MOVE_GEN);
	for(int pos = 0; pos < 64; pos++) if(position.color[pos] == position.side) {
		if(position.piece[pos] == PAWN) {
			// one forward
			char one_forward = (position.side == WHITE ? pos + 8 : pos - 8);
			int captured = EMPTY;
			if(row(one_forward) == 0 || row(one_forward) == 7) captured = PAWN;
			if(position.color[one_forward] == EMPTY) add_move(position, pos, one_forward);
			// two forward
			if((position.side == WHITE && row(pos) == 1) || (position.side == BLACK && row(pos) == 7)) {
				char two_forward = (position.side == WHITE ? pos + 16 : pos - 16);
				if(position.color[one_forward] == EMPTY && position.color[two_forward] == EMPTY) {
					add_move(position, pos, two_forward);
				}
			}
			// eating
			if(valid_distance(pos, one_forward - 1) && position.color[one_forward - 1] == position.xside) {
				add_move(position, pos, one_forward - 1);
			}
			if(valid_distance(pos, one_forward + 1) && position.color[one_forward + 1] == position.xside) {
				add_move(position, pos, one_forward + 1);
			}
			// enpassant
			if((position.side == WHITE && row(pos) == 4) || (position.side == BLACK && row(pos) == 3)) {
				// to the left
				if(col(pos) > 0 && position.color[pos - 1] == position.xside && position.piece[pos - 1] == PAWN && position.enpassant == col(pos - 1)) {
					if(position.side == WHITE) add_move(position, pos, pos + 7);
					else add_move(position, pos, pos - 9);
				// to the right
				} else if(col(pos) < 7 && position.color[pos + 1] == position.xside && position.piece[pos + 1] == PAWN && position.enpassant == col(pos + 1)) {
					if(position.side == WHITE) add_move(position, pos, pos + 9);
					else add_move(position, pos, pos - 7);
				}
			}
		} else {
			if(position.piece[pos] == KING) {
				// castling
				if(position.side == WHITE && (position.castling & 4)) {
					if((position.castling & 1) && position.move_valid(4, 2) == 0) add_move(position, 4, 2);
					if((position.castling & 2) && position.move_valid(4, 6) == 0) add_move(position, 4, 6);
				} else if(position.side == BLACK && position.castling & 32) {
					if((position.castling & 8) && position.move_valid(60, 58) == 0) add_move(position, 60, 58);
					if((position.castling & 16) && position.move_valid(60, 62) == 0) add_move(position, 60, 62);
				}
			}

			for(int i = 0; i < 8; i++) {
				if(offset[position.piece[pos]][i] == 0) break;
        		int new_pos = pos + offset[position.piece[pos]][i];

				if(!slide[position.piece[pos]]) {
					if(valid_distance(pos, new_pos) && position.color[new_pos] != position.side) {
						add_move(position, pos, new_pos);
					}
				} else {
					int prev_pos = pos;
					while(valid_distance(prev_pos, new_pos) && position.color[new_pos] != position.side) {
						add_move(position, pos, new_pos);
						if(position.color[new_pos] == position.xside) break;
						prev_pos = new_pos;
						new_pos += offset[position.piece[pos]][i];
					}
				}
			}
		}
	}
	order_and_push();
}