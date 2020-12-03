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

	if(is_attacked(from_sq, xside)) return false;

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
		while(++from_sq < to_sq) 
			if(piece[from_sq] != EMPTY) return false;	
	} else {
		if(is_attacked(from_sq - 1, xside)) return false;
		while(--from_sq > to_sq)
			if(piece[from_sq] != EMPTY) return false;
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

	return true;
}

void Position::make_move(Move move) {
	int from_sq = from(move);
	int to_sq = to(move); 
	
	assert(piece[from_sq] != EMPTY);
	assert(color[to_sq] != side);
	assert(check_coloring());

	const int p = piece[from_sq];

	if(p == PAWN && col(from_sq) != col(to_sq) && piece[to_sq] == EMPTY) {
		// enpasant
		assert(check_enpassant(move));
		int adjacent = 8 * row(from_sq) + col(to_sq);
		piece[adjacent] = EMPTY;
		color[adjacent] = EMPTY;
	} else if(p == PAWN && (row(to_sq) == 0 || row(to_sq) == 7)) {
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
	} else if(p == KING && abs(col(from_sq) - col(to_sq)) == 2) {
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
	} else if(p == PAWN && abs(row(from_sq) - row(to_sq)) == 2) {
		// change enpassant flag
		enpassant = col(from_sq);
	} else if(p == KING) {
		// change castling flag
		if(side == WHITE && (castling & 4)) {
			castling ^= 4;
		} else if(side == BLACK && (castling & 32)) {
			castling ^= 32;
		}
	} else if(p == ROOK) {
		// change castling flag
		if(from_sq == A1 && (castling & 1)) {
			castling ^= 1;
		} else if(from_sq == H1 && (castling & 2)) {
			castling ^= 2;
		} else if(from_sq == A8 && (castling & 8)) {
			castling ^= 8;
		} else if(from_sq == H8 && (castling & 16)) {
			castling ^= 16;
		}
	}

	if(p == PAWN && abs(row(from_sq) - row(to_sq)) == 2) {
		enpassant = col(from_sq);
	} else {
		enpassant = NO_ENPASSANT;
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

	if(from_sq == to_sq) return false;
	if(color[to_sq] == side) {
		return false;
	}
	if(piece[from_sq] == EMPTY) return false;
	if(side != color[from_sq]) return false;
	if(color[from_sq] == color[to_sq]) return false;
	if(!valid_pos(from_sq) || !valid_pos(to_sq)) return false;

	Position position_backup = *this;

	const int p = piece[from_sq];
	const int c = color[from_sq];

	bool enpassant_checked = false;

	if(p == KING && abs(col(from_sq) - col(to_sq)) == 2) {
		if(!check_castling(move)) return false;
	} else if(p == PAWN && col(from_sq) != col(to_sq) && piece[to_sq] == EMPTY) {
		if(check_enpassant(move) == false) return false;
		enpassant_checked = true;
	} else if(slide[p]) {
		switch(p) {
			case PAWN: {
				int forward = (side == WHITE ? (from_sq + 8) : (from_sq -8));
				int two_forward = (side == WHITE ? (from_sq + 16) : (from_sq - 16));
				if(to_sq == forward + 1 || to_sq == forward - 1) {
					if(piece[to_sq] == EMPTY) return false;
				} else if(to_sq == forward) {
					if(piece[forward] != EMPTY) return false;
				} else if(to_sq == two_forward) {
					if((side == WHITE && row(from_sq) != 1) || (side == BLACK && row(from_sq) == 6)) return false;
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
				if(distance(from_sq, to_sq) > 2 || color[to_sq] == color[from_sq]) return false;
				break;
			}
			default:
				assert(false);
				break;
		}
	} else if(slide[p]) { 
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

	bool ok = true;
	Position prev_position = *this;
	make_move(move);
	if(in_check(c)) ok = false;
	*this = prev_position; // take back

	return ok;
} 