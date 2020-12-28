#include <cassert>
#include <iostream>
#include "defs.h"
#include "data.h"
#include "board.h"

bool Position::check_coloring() {
	for(int i = 0; i < 64; i++) {
		if(piece[i] == EMPTY && color[i] != EMPTY) return false;
		if(piece[i] != EMPTY && color[i] == EMPTY) return false;
	}
	return true;
}

bool Position::check_castling(Move move) {
	int from_sq = from(move);
	int to_sq = to(move);

	if(is_attacked(from_sq, xside) || is_attacked(to_sq, xside))
		return false;

	bool white_can_castle = castling & 4;  
	bool black_can_castle = castling & 32;

	bool white_queen_side = (side == WHITE) && (from_sq == 4 && to_sq == 2) && (castling & 1) && white_can_castle;
	bool white_king_side  = (side == WHITE) && (from_sq == 4 && to_sq == 6) && (castling & 2) && white_can_castle; 
	bool black_queen_side = (side == BLACK) && (from_sq == 60 && to_sq == 58) && (castling & 8) && black_can_castle;
	bool black_king_side  = (side == BLACK) && (from_sq == 60 && to_sq == 62) && (castling & 16) && black_can_castle; 

	if(!white_queen_side
	&& !white_king_side
	&& !black_queen_side
	&& !black_king_side)
		return false;

	if(white_king_side || black_king_side) {
		if(is_attacked(from_sq + 1, xside)) return false;
		if(piece[from_sq + 1] != EMPTY) return false;
		if(piece[from_sq + 2] != EMPTY) return false;
	} else {
		if(is_attacked(from_sq - 1, xside)) return false;
		if(piece[from_sq - 1] != EMPTY) return false;
		if(piece[from_sq - 2] != EMPTY) return false;
		if(piece[from_sq - 3] != EMPTY) return false;
	}

	return true; // later we will check whether the king is attacked after castling 
}

bool Position::check_enpassant(Move move) {
	int from_sq = from(move);
	int to_sq = to(move);

	assert(color[to_sq] == EMPTY);
	assert(piece[to_sq] == EMPTY);

	int forward = (side == WHITE ? (from_sq + 8) : (from_sq - 8));	
	int adjacent = 8 * row(from_sq) + col(to_sq);

	assert(col(adjacent) == col(to_sq));

	if(piece[adjacent] != PAWN || color[adjacent] != xside) 
		return false;

	// it's not moving diagonally
	if(to_sq != forward + 1 && to_sq != forward - 1)
		return false;

	// the pawn can't be eaten enpassant
	if(enpassant != col(adjacent))
		return false;

	// it's eating a different piece
	if(side == WHITE && row(to_sq) != 5) 
		return false;

	if(side == BLACK && row(to_sq) != 2)
		return false;

	if(distance(from_sq, to_sq) > 2)
		return false;

	return true;
}

void Position::make_move(Move move) {
	int from_sq = from(move);
	int to_sq = to(move); 
	
	assert(piece[from_sq] != EMPTY);
	assert(color[to_sq] != side);
	assert(check_coloring());

	const int p = piece[from_sq];

	if(p == KING && abs(col(from_sq) - col(to_sq)) == 2) {
		// castling
		assert(check_castling(move));
		assert(!is_attacked(to_sq, xside));

		bool white_queen_side = (side == WHITE) && (from_sq == E1 && to_sq == C1) && (castling & 1) && (castling & 4);
		bool white_king_side  = (side == WHITE) && (from_sq == E1 && to_sq == G1) && (castling & 2) && (castling & 4); 
		bool black_queen_side = (side == BLACK) && (from_sq == E8 && to_sq == C8) && (castling & 8) && (castling & 32);
		bool black_king_side  = (side == BLACK) && (from_sq == E8 && to_sq == G8) && (castling & 16) && (castling & 32); 

		if(white_queen_side) {
			castling ^= 1 | 4;
			piece[A1] = EMPTY; color[A1] = EMPTY;
			piece[D1] = ROOK;  color[D1] = WHITE;
		} else if(white_king_side) {
			castling ^= 2 | 4;
			piece[H1] = EMPTY; color[H1] = EMPTY;
			piece[F1] = ROOK;  color[F1] = WHITE;
		} else if(black_queen_side) {
			castling ^= 8 | 32;
			piece[A8] = EMPTY; color[A8] = EMPTY;
			piece[D8] = ROOK;  color[D8] = BLACK;
		} else if(black_king_side) {
			castling ^= 16 | 32;
			piece[H8] = EMPTY; color[H8] = EMPTY;
			piece[F8] = ROOK;  color[F8] = BLACK;
		}
	}

	if((castling & 1) && (from_sq == A1 || to_sq == A1)) {
		// std::cout << "Changing castling flag 1 with move " << move_to_str(move) << endl;
		// print_board();
		castling ^= 1;
	}
	if((castling & 2) && (from_sq == H1 || to_sq == H1)) {
		// std::cout << "Changing castling flag 2 with move " << move_to_str(move) << endl;
		// print_board();
		castling ^= 2;
	}
	if((castling & 8) && (from_sq == A8 || to_sq == A8)) {
		// std::cout << "Changing castling flag 8 with move " << move_to_str(move) << endl;
		// print_board();
		castling ^= 8;
	}
	if((castling & 16) && (from_sq == H8 || to_sq == H8)) {
		// std::cout << "Changing castling flag 16 with move " << move_to_str(move) << endl;
		// print_board();
		castling ^= 16;
	}

	if((castling & 4) && from_sq == E1) {
		// std::cout << "Changing castling flag 4 with move " << move_to_str(move) << endl;
		// print_board();
		castling ^= 4;
	}

	if((castling & 32) && from_sq == E8) { 
		// std::cout << "Changing castling flag 32 with move " << move_to_str(move) << endl;
		// print_board();
		castling ^= 32;
	}


	if(p == PAWN && col(from_sq) != col(to_sq) && piece[to_sq] == EMPTY) {
		// enpasant
		assert(check_enpassant(move));
		int adjacent = 8 * row(from_sq) + col(to_sq);
		piece[adjacent] = EMPTY;
		color[adjacent] = EMPTY;
	}

	enpassant = NO_ENPASSANT;

	if(p == PAWN && (row(to_sq) == 0 || row(to_sq) == 7)) {
		// promotion
		piece[from_sq] = EMPTY;
		color[from_sq] = EMPTY;
		piece[to_sq] = QUEEN;
		color[to_sq] = side;
		move_cnt++;
		side = !side;
		xside = !xside;
		assert(check_coloring());

		return;
	}
	
	if(p == PAWN && abs(row(from_sq) - row(to_sq)) == 2) {
		// change enpassant flag
		enpassant = col(from_sq);
	}

	// normal move
	assert(color[to_sq] != side);
	piece[to_sq] = piece[from_sq];
	color[to_sq] = side;
	piece[from_sq] = EMPTY;
	color[from_sq] = EMPTY;

	side = !side;
	xside = !xside;
	move_cnt++;

	assert(check_coloring());
}

bool Position::move_valid(Move move) {
	int from_sq = from(move);
	int to_sq = to(move);

	/*
	if(move_to_str(move) == "a5h3") {
		std::cout << BLUE_COLOR;
		std::cout << "Checking move_valid in old board " << move_to_str(move) << endl;
		print_board();
		std::cout << RESET_COLOR;
	}
	*/

	assert(check_coloring());

	if(from_sq == to_sq
	|| color[to_sq] == side 
	|| piece[from_sq] == EMPTY
	|| side != color[from_sq]
	|| color[from_sq] == color[to_sq]
	|| !valid_pos(from_sq)
	|| !valid_pos(to_sq)) {
		return false;
	}

	Position position_backup = *this;

	const int p = piece[from_sq];
	const int c = color[from_sq];

	bool enpassant_checked = false;

	if(p == KING && abs(col(from_sq) - col(to_sq)) == 2) {
		if(!check_castling(move)) return false;
	} else if(p == PAWN && col(from_sq) != col(to_sq) && piece[to_sq] == EMPTY) {
		/*
		if(move_to_str(move) == "a5h3") {
			std::cout << "Checking if enpassant is valid" << endl;
		}
		*/
		if(!check_enpassant(move)) return false;
		enpassant_checked = true;
	} else {
		if(!slide[p]) {
			assert(p == PAWN || p == KNIGHT || p == KING);
			switch(p) {
				case PAWN: {
					int forward = (side == WHITE ? (from_sq + 8) : (from_sq - 8));
					int two_forward = (side == WHITE ? (from_sq + 16) : (from_sq - 16));
					if(to_sq == forward + 1 || to_sq == forward - 1) {
						if(distance(from_sq, to_sq) != 2) return false;
						if(piece[to_sq] == EMPTY) return false;
					} else if(to_sq == forward) {
						if(piece[forward] != EMPTY) return false;
					} else if(to_sq == two_forward) {
						if((side == WHITE && row(from_sq) != 1) || (side == BLACK && row(from_sq) != 6)) return false;
						if(piece[forward] != EMPTY || piece[two_forward] != EMPTY) return false;
					} else {
						return false;
					}
					break;
				}
				case KNIGHT: {
					assert(p != BISHOP);
					bool two = false, one = false;
					if(abs(row(from_sq) - row(to_sq)) == 2) two ^= 1;
					if(abs(row(from_sq) - row(to_sq)) == 1) one ^= 1;
					if(abs(col(from_sq) - col(to_sq)) == 2) two ^= 1;
					if(abs(col(from_sq) - col(to_sq)) == 1) one ^= 1;
					if(!two || !one) return false;
					break;
				}
				case KING: {
					if(color[to_sq] == color[from_sq])
						return false;
					if(col(from_sq) == col(to_sq) || row(from_sq) == row(to_sq)) {
						if(distance(from_sq, to_sq) > 1)
							return false;
					} else {
						if(distance(from_sq, to_sq) > 2)
							return false;
					}
					break;
				}
			}
		} else { 
			assert(p == BISHOP || p == ROOK || p == QUEEN);
			bool ok = false;
			for(int i = 0; !ok && offset[p][i] != 0; i++) {
				int delta = offset[p][i];
				int pos = from_sq;
				while(!ok && distance(pos, pos + delta) <= 2 && valid_pos(pos + delta)) {
					if(color[pos + delta] == c) break;
					if(pos + delta == to_sq) ok = true;
					if(color[pos + delta] != EMPTY) break;
					pos = pos + delta;
				}
			}
			if(!ok) return false;
		}
	}

	bool ok = true;
	Position prev_position = *this;
	make_move(move);
	if(in_check(c)) ok = false;
	*this = prev_position; // take back

	return ok;
} 
