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

// we assume that the move is valid (because we have checked for validity before)
Move Position::make_move(char from, char to, char promotion_piece = QUEEN) {
	if(piece[from] == EMPTY)
		throw("No piece at position 'from'");

  	char prev_enpassant = enpassant; // store previous flags
  	char prev_castling = castling;
	char captured = EMPTY;
	enpassant = 8; //reset the enpassant flag

	if(piece[from] == PAWN && piece[to] == EMPTY && col(from) != col(to)) {
		// eat enpassant
		char adjacent = row(from) * 8 + col(to);
		if(piece[adjacent] != PAWN)
			throw("Adjacent piece at enpassant isn't a pawn");
		if(color[adjacent] == color[from] || color[adjacent] == EMPTY)
			throw("Adjacent piece doesn't have the right color at enpasssant");

		piece[adjacent] = EMPTY;
		color[adjacent] = EMPTY;
		piece[to] = piece[from];
		color[to] = color[from];
		piece[from] = EMPTY;
		color[from] = EMPTY;

		side ^= 1;
		xside ^= 1;
		move_cnt++;
		if(check_coloring() == false)
			throw("Invalid coloring after enpassant");
		return Move(from, to, EMPTY, prev_castling, prev_enpassant);

	} else if(piece[from] == PAWN && (row(to) == 0 || row(to) == 7)) {
    	// promote a pawn
		if(col(to) != col(from)) {
			if(abs(col(to) - col(from)) != 1)
				throw("Pawn promotion by eating but pawn isn't eating diagonally");
			if(color[from] == color[to] || color[to] == EMPTY)
				throw("Pawn is eating the wrong color or an empty square at promotion");
		}

		captured = piece[to];
		piece[to] = promotion_piece;
		color[to] = side;
		piece[from] = EMPTY;
		color[from] = EMPTY;

		side ^= 1;
		xside ^= 1;
		move_cnt++;
		if(check_coloring() == false)
			throw("Invalid coloring after castling");
		return Move(from, to, captured, prev_castling, prev_enpassant, true);

	} else if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		// castling
		char rook_prev, rook_now;

		if(from == 4 && to == 2) { rook_prev = 0; rook_now = 3; castling ^= 1; castling ^= 4; }
		else if(from == 4 && to == 6) { rook_prev = 7; rook_now = 5; castling ^= 2; castling ^= 4; }
		else if(from == 60 && to == 58) { rook_prev = 56; rook_now = 59; castling ^= 8; castling ^= 32; }
		else if(from == 60 && to == 62) { rook_prev = 63; rook_now = 61; castling ^= 16; castling ^= 32; }
		else throw("Invalid castling move"); 

		piece[rook_prev] = EMPTY;
		color[rook_prev] = EMPTY;
		piece[rook_now] = ROOK;
		color[rook_now] = side;

	} else {
		// anything else
		if(piece[from] == KING) {
			// disable castling flag for the king
			if(side == WHITE && (castling & 4)) castling ^= 4;
			else if(side == BLACK && (castling & 32)) castling ^= 32;

		} else if(piece[from] == ROOK || piece[to] == ROOK) {
			// disable castling flag for this rook
			if((to == 0 || from == 0) && (castling & 1)) castling ^= 1;
			else if((to == 7 || from == 7) && (castling & 2)) castling ^= 2;
			else if((to == 54 || from == 54) && (castling & 8)) castling ^= 8;
			else if((to == 63 || from == 63) && (castling & 16)) castling ^= 16;

		} else if(piece[from] == PAWN && abs(row(from) - row(to)) == 2) {
			// pawn moving two squares
			enpassant = col(from);
		}
	}

	captured = piece[to];
	piece[to] = piece[from];
	color[to] = color[from];
	piece[from] = EMPTY;
	color[from] = EMPTY;

	if(piece[0] != ROOK && (castling & 1)) castling ^= 1;
	if(piece[7] != ROOK && (castling & 2)) castling ^= 2;
	if(piece[56] != ROOK && (castling & 8)) castling ^= 8;
	if(piece[63] != ROOK && (castling & 16)) castling ^= 16;

	side ^= 1;
	xside ^= 1;
	move_cnt++;
	if(check_coloring() == false)
		throw("Invalid coloring after making move");
	return Move(from, to, captured, prev_castling, prev_enpassant);
}

void Position::make_move(Move move) {
	make_move(move.from, move.to, QUEEN);
}

void Position::take_back(Move m) {
	assert(piece[m.from] == EMPTY);
	assert(color[m.from] == EMPTY);
	assert(piece[m.to] != EMPTY);
	assert(color[m.to] != EMPTY);

  	if(m.promotion) {
		// promotion
		piece[m.from] = PAWN;
		color[m.from] = color[m.to];
		piece[m.to] = m.captured;
		if(m.captured == EMPTY) color[m.to] = EMPTY;
		else color[m.to] = (color[m.from] == WHITE ? BLACK : WHITE);

  	} else if(piece[m.to] == PAWN && col(m.from) != col(m.to) && m.captured == EMPTY) {
		// eat enpassant
		assert(piece[m.to] == PAWN);
		if(piece[m.to] != PAWN)
			throw("Pawn captured enpassant but there is a pawn at 'to'");
		piece[m.from] = PAWN;
		color[m.from] = color[m.to];
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;
		char adjacent = 8 * row(m.from) + col(m.to);
		piece[adjacent] = PAWN;
		color[adjacent] = (color[m.from] == BLACK ? WHITE : BLACK);

  	} else if(piece[m.to] == KING && abs(col(m.to) - col(m.from)) == 2) {
		// castling
		char rook_prev, rook_now;
		if(m.from == 4 && m.to == 2) { rook_prev = 0; rook_now = 3; }
		else if(m.from == 4 && m.to == 6) { rook_prev = 7; rook_now = 5; }
		else if(m.from == 60 && m.to == 58) { rook_prev = 56; rook_now = 59; }
		else if(m.from == 60 && m.to == 62) { rook_prev = 63; rook_now = 61; }
		else throw("Invalid castling move at take_back");

		// move back the king
		if(piece[m.to] != KING)
			throw("There's no king at to but move is flagged as castling");
		piece[m.from] = KING;
		color[m.from] = color[m.to];
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;
		// move back the rook
		if(piece[rook_now] != ROOK)
			throw("Taking back castling move but rook isn't where it should be");
		piece[rook_prev] = ROOK;
		color[rook_prev] = color[m.from]; //same color as king
		piece[rook_now] = EMPTY;
		color[rook_now] = EMPTY;

	} else {
		// anything else
		piece[m.from] = piece[m.to];
	  	color[m.from] = color[m.to];

		piece[m.to] = m.captured;
		if(m.captured == EMPTY) color[m.to] = EMPTY;
		else color[m.to] = (color[m.from] == WHITE ? BLACK : WHITE);
	}

	if(m.captured != EMPTY) {
		if(piece[m.to] == EMPTY)
			throw("Captured piece but captured flag is marked as empty");
		if(color[m.to] == EMPTY)
			throw("Captured piece but captured flag is marked as empty");
	}

	castling = m.castling;
	enpassant = m.enpassant;
	side ^= 1;
	xside ^= 1;
	move_cnt--;
}

// comparing the two functions for catching bugs
bool Position::move_valid(char from, char to) {
	bool old_version = (old_move_valid(from, to) == 0);
	bool new_version = new_move_valid(from, to);
	bool special_move = piece[from] == BISHOP && distance(from, to) == 3;
	if(new_version != old_version && !special_move) {
		std::cout << '\n' << '\n' << "Both functions don't match" << endl;
		std::cout << "Move is " << str_move(from, to) << '\n';
		std::cout << "Enpassant is " << int(enpassant) << '\n';
		std::cout << "Castling is " << int(castling) << '\n';
		std::cout << "Check enpassant returns " << check_enpassant(from, to) << '\n';
		std::cout << "Board is: " << '\n';
		print_board();
		std::cout << "New version says that move is " << (new_version == true ? "valid" : "invalid") << '\n';
		std::cout << "Old version says that move is " << (old_version == true ? "valid" : "invalid") << '\n';
		throw("Move valid checking differs between old and new function");
	}
	return new_version;
}

// used by the new function
bool Position::check_castling(char from, char to) {
	if(is_attacked(from, xside)) return false;

	bool white_can_castle = castling & 4;  
	bool black_can_castle = castling & 32;

	bool white_queen_side = (side == WHITE) && (from == 4 && to == 2) && (castling & 1) && white_can_castle;
	bool white_king_side  = (side == WHITE) && (from == 4 && to == 6) && (castling & 2) && white_can_castle; 
	bool black_queen_side = (side == BLACK) && (from == 60 && to == 58) && (castling & 8) && black_can_castle;
	bool black_king_side  = (side == BLACK) && (from == 60 && to == 62) && (castling & 16) && black_can_castle; 

	if(!white_queen_side
	&& !white_king_side
	&& !black_queen_side
	&& !black_king_side)
		return false;

	if(white_king_side || black_king_side) {
		if(is_attacked(from + 1, xside)) return false;
		while(++from < to) 
			if(piece[from] != EMPTY) return false;	
	} else {
		if(is_attacked(from - 1, xside)) return false;
		while(--from > to)
			if(piece[from] != EMPTY) return false;
	}

	return true; // later we will check whether the king is attacked after castling 
}

bool Position::check_enpassant(char from, char to) {
	assert(color[to] == EMPTY);
	assert(piece[to] == EMPTY);

	int forward = (side == WHITE ? (from + 8) : (from - 8));	
	int adjacent = 8 * row(from) + col(to);

	assert(col(adjacent) == col(to));

	// it's not moving diagonally
	if(to != forward + 1 && to != forward - 1)
		return false;

	// the pawn can't be eaten enpassant
	if(enpassant != col(adjacent))
		return false;

	// it's eating a different piece
	if(side == WHITE && row(to) != 5) 
		return false;

	if(side == BLACK && row(to) != 2)
		return false;

	return true;
}

bool Position::new_move_valid(char from, char to) {
	if(from == to) return false;
	if(piece[from] == EMPTY) return false;
	if(side != color[from]) return false;
	if(color[from] == color[to]) return false;
	if(!valid_pos(from) || !valid_pos(to)) return false;

	const int p = piece[from];
	const int c = color[from];

	if(!slide[p] && !valid_distance(from, to)) return false;

	if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		if(!check_castling(from, to)) return false;
	} else if(piece[from] == PAWN && abs(col(from) - col(to)) == 1 && abs(row(from) - row(to)) == 1 && piece[to] == EMPTY) {
		if(check_enpassant(from, to) == false) return false;
	} else {
		switch(p) {
			case PAWN: {
				int forward = (side == WHITE ? (from + 8) : (from -8));
				int two_forward = (side == WHITE ? (from + 16) : (from - 16));
				if(to == forward + 1 || to == forward - 1) {
					if(piece[to] == EMPTY) return false;
				} else if(to == forward) {
					if(piece[forward] != EMPTY) return false;
				} else if(to == two_forward) {
					if((side == WHITE && row(from) != 1) || (side == BLACK && row(from) == 6)) return false;
					if(piece[forward] != EMPTY || piece[two_forward] != EMPTY) return false;
				} else {
					return false;
				}
				break;
			}
			case KNIGHT: {
				assert(p != BISHOP);
				bool two = false, one = false;
				if(abs(row(from) - row(to)) == 2) two ^= 1;
				if(abs(row(from) - row(to)) == 1) one ^= 1;
				if(abs(col(from) - col(to)) == 2) two ^= 1;
				if(abs(col(from) - col(to)) == 1) one ^= 1;
				if(!two || !one) return false;
				break;
			}
			case KING: {
				if(distance(from, to) > 2 || color[to] == color[from]) return false;
				break;
			}
			default:
				assert(p == BISHOP || p == ROOK || p == QUEEN);
				break;
		}
	}
	
	if(slide[p]) {
		assert(p == BISHOP || p == ROOK || p == QUEEN);
		bool ok = false;
		for(int i = 0; !ok && offset[p][i] != 0; i++) {
			int delta = offset[p][i];
			int pos = from;
			while(!ok && distance(pos, pos + delta) <= 2 && valid_pos(pos + delta)) {
				if(color[pos + delta] == c) break;
				if(pos + delta == to) ok = true;
				if(color[pos + delta] != EMPTY) break;
				pos = pos + delta;
			}
		}
		if(!ok) return false;
	}

	bool ok = true;
	Move move = make_move(from, to);
	if(in_check(c)) ok = false;
	take_back(move);

	if(!ok) return false; // in check
	return true;
} 

int Position::old_move_valid(char from, char to) {
	if(from == to) return 1;
	else if(piece[from] == EMPTY) return 2;
	else if(side != color[from]) return 3;
	else if(color[from] == color[to]) return 4;

  	if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		// castling

		// following conditions must hold:
		// (1) King hasn't moved
		// (2) Rook of specific side hasn't moved
		// (3) King isn't in check
		// (4) There are no pieces in between
		// (5) The king can't be checked on the squares in between

		// check flags
		char rook_pos;
		if(side == WHITE && from == 4 && to == 2 && (castling & 1) && (castling & 4)) rook_pos = 0;
		else if(side == WHITE && from == 4 && to == 6 && (castling & 2) && (castling & 4)) rook_pos = 7;
		else if(side == BLACK && from == 60 && to == 58 && (castling & 8) && (castling & 32)) rook_pos = 56;
		else if(side == BLACK && from == 60 && to == 62 && (castling & 16) && (castling & 32)) rook_pos = 63;
		else return 5;

		char pos = from;
		int direction = (to > from ? 1 : -1);
    	bool done = false;

		// no pieces should be in between
		for(pos = from + direction; !done; pos += direction) {
			if(pos == rook_pos) break;
			if(piece[pos] != EMPTY) return 6;
		}

		// king isn't in check and can't be check on his way to "to"
		for(pos = from; !done; pos += direction) {
			if(is_attacked(pos, xside)) return 7;
			if(pos == to) break;
		}

		return 0; // no need for checking for check after move

	} else if(piece[from] == PAWN) {
		// pawn
		if(side == WHITE && to < from) return 8;
		else if(side == BLACK && to > from) return 9;

		// difference in rows and cols
		int delta_row = abs(row(to) - row(from));
		int delta_col = abs(col(to) - col(from));

		if(delta_row == 1 && delta_col == 1 && color[to] == EMPTY) {
      		// eat enpassant
			char adjacent = row(from) * 8 + col(to);
			if(piece[to] != EMPTY || color[adjacent] != xside) return 10;
			else if(enpassant != col(to)) return 11;
		}
		else if(delta_row == 1 && delta_col == 0 && piece[to] == EMPTY) {}
		else if(delta_row == 2 && delta_col == 0 && piece[to] == EMPTY && piece[from + (side == WHITE ? 8 : -8)] == EMPTY) {} 
		else if(delta_row == 1 && delta_col == 1 && color[to] != EMPTY && color[to] == xside) {}
		else return 13; // the move is invalid

	} else {
		// any other movement

		bool valid = false;

		for(int i = 0; i < 8 && !valid; i++) {
			if(offset[piece[from]][i] == 0) break;

			char prev_pos = from, new_pos = from + offset[piece[from]][i];
			if(!valid_pos(new_pos) || distance(prev_pos, new_pos) > 3) continue;

			while(slide[piece[from]] && valid_pos(new_pos) && new_pos != to && color[new_pos] == EMPTY && distance(prev_pos, new_pos) <= 3) {
				prev_pos = new_pos;
				new_pos += offset[piece[from]][i];
			}

			if(to == new_pos) valid = true;
		}
		if(!valid) return 14;
	}

	Move m = make_move(from, to);
	bool valid = true;
	if(in_check(xside)) valid = false; // remember that side gets flipped
	take_back(m);

	if(valid) return 0; // move is valid
  	else return 15; // error code
}