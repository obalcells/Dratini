#include <iostream>
#include <cassert>
#include "data.h"
#include "protos.h"

int distance(int pos_1, int pos_2) {
	return abs(row(pos_1) - row(pos_2)) + abs(col(pos_1) - col(pos_2));
}
bool valid_pos(int pos) {
	return (pos >= 0 && pos < 64) ? true : false;
}
bool valid_move(int pos_1, int pos_2) {
	return pos_2 >= 0 && pos_2 < 64 && distance(pos_1, pos_2) <= 3;
}
// is position {pos} being attacked by color {e_color} ?
bool is_attacked(int pos, int e_color = EMPTY) {
	if(e_color == EMPTY) {
		// we have to know who is the attacker
		if(color[pos] != EMPTY) e_color = color[pos] == WHITE ? BLACK : WHITE;
		else assert(e_color != EMPTY);
	}

	//pawns
	if(e_color == BLACK) {
		if(valid_move(pos, pos + 7) && piece[pos + 7] == PAWN && color[pos + 7] == e_color) return true;
		if(valid_move(pos, pos + 9) && piece[pos + 9] == PAWN && color[pos + 9] == e_color) return true;
	} else if(e_color == WHITE) {
		if(valid_move(pos, pos - 7) && piece[pos + 7] == PAWN && color[pos + 7] == e_color) return true;
		if(valid_move(pos, pos - 9) && piece[pos + 9] == PAWN && color[pos + 9] == e_color) return true;
	}

	int new_pos = pos, prev_pos;

	//knight's offset
	for(int i = 0; i < 8; i++) {
		if(offset[KNIGHT][i] == 0) break;
		if(!valid_move(pos, pos + offset[KNIGHT][i])) continue;
		if(piece[pos + offset[KNIGHT][i]] == KNIGHT && color[pos + offset[KNIGHT][i]] == e_color) return true;
	}

	//bishop's offset
	for(int i = 0; i < 8; i++) {
		if(offset[BISHOP][i] == 0) break;
		if(!valid_move(pos, pos + offset[BISHOP][i])) continue;
		new_pos = pos + offset[BISHOP][i]; prev_pos = pos;
		while(valid_move(new_pos, new_pos + offset[BISHOP][i]) && color[new_pos] == EMPTY) new_pos += offset[BISHOP][i];
		assert(valid_pos(new_pos));
		if((piece[new_pos] == BISHOP || piece[new_pos] == QUEEN) && color[new_pos] == e_color) return true;
	}

	//rook's offset
	for(int i = 0; i < 8; i++) {
		if(offset[ROOK][i] == 0) break;
		if(!valid_move(pos, pos + offset[ROOK][i])) continue;
		new_pos = pos + offset[ROOK][i]; prev_pos = pos;
		while(valid_move(new_pos, new_pos + offset[ROOK][i]) && color[new_pos] == EMPTY) new_pos += offset[ROOK][i];
		assert(valid_pos(new_pos));
		if((piece[new_pos] == ROOK || piece[new_pos] == QUEEN) && color[new_pos] == e_color) return true;
	}

	return false;
}
// checks if King of the side "side" is under attack
bool is_check(int _side) {
	for(int i = 0; i < 64; i++) {
		if(piece[i] == KING && color[i] == _side) {
			return is_attacked(i, _side ^ 1);
		}
	}
}

/*
struct Move {
	char from;
	char to;
	char captured;
}
*/

// we alter the state and return a struct representing the move
// we assume that the movement is valid (but there are some asserts anyways)
Move make_move(int from, int to, int promotion_piece = QUEEN) {
	enpassant = 0; //reset the enpassant flag

	if(piece[from] == PAWN && piece[to] == EMPTY && col(from) != col(to)) {
		// eat enpassant way
		int adjacent = row(from) * 8 + col(to);
		assert(piece[adjacent] == PAWN);
		assert(color[adjacent] != color[from]);
		piece[adjacent] = EMPTY;
		color[adjacent] = EMPTY;

	} else if(piece[from] == PAWN && (row(to) == 0 || row(to) == 7)) {
		// promote a pawn
		captured = PAWN;
		piece[to] = promotion_piece;
		color[to] = side;

	} else if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		// castling
		int rook_prev = 0, rook_now = -1;
		if(from == 4 && to == 2) { rook_prev = 0; rook_now = 3; castling ^= 1; castling ^= 4; }
		else if(from == 4 && to == 6) { rook_prev = 7; rook_now = 5; castling ^= 2; castling ^= 4; }
		else if(from == 60 && to == 58) { rook_prev = 56; rook_now = 59; castling ^= 8; castling ^= 32; }
		else if(from == 60 && to == 62) { rook_prev = 63; rook_now = 61; castling ^= 16; castling ^= 32; }
		else assert(false); // move shouldn't be valid if we got here

		piece[rook_prev] = EMPTY;
		color[rook_prev] = EMPTY;
		piece[rook_now] = ROOK;
		color[rook_now] = side;

	} else {
		// anything else
		if(piece[from] == KING) {
			// disable castling flag for the king
			if(side == 0 && (castling & 4)) castling ^= 4;
			else if(side == 1 && (castling & 32)) castling ^= 32;

		} else if(piece[from] == ROOK) {
			// disable castling flag for this rook
			if(side == 0 && from == 0 && (castling << 1)) castling ^= 1;
			else if(side == 0 && from == 7 && (castling << 2)) castling ^= 2;
			else if(side == 1 && from == 54 && (castling << 8)) castling ^= 8;
			else if(side == 1 && from == 63 && (castling << 16)) castling ^= 16;

		} else if(piece[from] == PAWN && abs(row(from) - row(to)) == 2) {
			// pawn moving two squares
			if(side == 0) enpassant |= (1 << col(from));
		}
	}

	int captured = piece[to];
	piece[to] = piece[from];
	color[to] = side;
	piece[from] = EMPTY;
	color[from] = EMPTY;
	return Move({ char(from), char(to), char(captured) });
}

void undo_move(Move m) {
	assert(piece[m.from] == EMPTY);
	assert(color[m.from] == EMPTY);
	assert(piece[m.to] != EMPTY);
	assert(color[m.to] != EMPTY);

	if(piece[m.to] 
	if(piece[m.to] == PAWN && col(m.from) != col(m.to) && m.captured == EMPTY) {
		// enpassant
		assert(piece[m.to] == PAWN);
		piece[m.from] = PAWN;
		color[m.from] = color[m.to];

		assert(m.captured == EMPTY); //enpassant captured is marked as empty
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;

		int adjacent = 8 * row(m.from) + col(m.to);
		piece[adjacent] = PAWN;
		color[adjacent] = (color[m.from] == BLACK ? WHITE : BLACK);

	} else if(piece[m.to] == KING && abs(col(m.to) - col(m.from)) > 1) {
		// castling
		int rook_prev = -1, rook_now = -1;
		if(m.from == 4 && m.to == 2) { rook_prev = 0; rook_now = 3; }
		else if(m.from == 4 && m.to == 6) { rook_prev = 7; rook_now = 5; }
		else if(m.from == 60 && m.to == 58) { rook_prev = 56; rook_now = 59; }
		else if(m.from == 60 && m.to == 62) { rook_prev = 63; rook_now = 61; }

		// move back the king
		assert(piece[m.to] == KING);
		piece[m.from] = KING;
		color[m.from] = color[m.to];
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;
		// move back the rook
		assert(piece[rook_now] = ROOK);
		piece[rook_prev] = ROOK;
		color[rook_prev] = color[m.from]; //same color as king
		piece[rook_now] = EMPTY;
		color[rook_now] = EMPTY;
	} else {
		// anything else
		piece[m.from] = piece[m.to];
		color[m.from] = color[m.to];

		piece[m.to] = m.captured;
		color[m.to] = (piece[m.from] == WHITE ? BLACK : WHITE);
		if(piece[m.to] == EMPTY) color[m.to] = EMPTY;
	}
}

bool is_move_valid(int from, int to) {
	if(from == to) return false;
	if(piece[from] == EMPTY) return false;
	if(side != color[from]) return false;
	if(color[from] == color[to]) return false;

	if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		// CASTLING
		// Following conditions must hold:
		// (1) King hasn't moved
		// (2) Rook of specific side hasn't moved
		// (3) King isn't in check
		// (4) There are no pieces in between
		// (5) The king can't be checked on the squares in between

		// check flags
		int rook_pos = -1;
		if(side == 0 && from == 4 && to == 2 && (castling & 1) && (castling & 4)) rook_pos = 0;
		else if(side == 0 && from == 4 && to == 6 && (castling & 2) && (castling & 4)) rook_pos = 7;
		else if(side == 1 && from == 60 && to == 58 && (castling & 8) && (castling & 32)) rook_pos = 56;
		else if(side == 1 && from == 60 && to == 62 && (castling & 16) && (castling & 32)) rook_pos = 63;
		else return false;

		int direction = (to > from) ? 1 : -1;

		// no pieces in between
		for(int pos = from + direction;; pos += direction) {
			if(pos == rook_pos) break;
			if(piece[pos] != EMPTY) return false;
		}

		// king isn't in check and can't be check on his way to "to"
		for(int pos = from;; pos += direction) {
			if(is_attacked(pos, xside)) return false;
			if(pos == to) break;
		}

		return true;

	} else if(piece[from] == PAWN) {
		//PAWN MOVEMENTS

		//remember: side = 0 -> WHITE and initial_color[0] = WHITE
		if(side == 0 && to < from) return false;
		else if(side == 1 && to > from) return false;

		//difference in rows and cols
		int delta_row = abs(row(to) - row(from));
		int delta_col = abs(col(to) - col(from));

		if(delta_row == 1 && delta_col == 1 && color[to] == EMPTY) {
			int adjacent = row(from) * 8 + col(to);
			if(piece[to] != EMPTY || color[adjacent] != xside) return false;
			if(side == 0 && !(enpassant & (1 << col(to)))) return false;
			else if(side == 1 && !(enpassant & (1 << (8 + col(to))))) return false;
		}
		else if(delta_row == 1 && delta_col == 0 && piece[to] == EMPTY) { /* fine */ }
		else if(delta_row == 2 && delta_col == 0 && piece[to] == EMPTY && piece[from + (side == 0 ? 8 : -8)] == EMPTY) { /* fine too */ }
		else if(delta_row == 1 && delta_col == 1 && color[to] != EMPTY && color[to] == xside) { /* eating is fine */ }
		else return false; /* not fine */

		Move m = make_move(from, to);
		bool valid = true;
		//if(is_check(side)) valid = false;
		//undo_move(m);
		//if(valid) return true;
		//else return false;

	} else {
		//NORMAL MOVEMENT

		bool valid = false;

		for(int i = 0; i < 8 && !valid; i++) {
			if(offset[piece[from]][i] == 0) break;

			int prev_pos = from, new_pos = from + offset[piece[from]][i];
			if(!valid_pos(new_pos) || distance(prev_pos, new_pos) > 3) continue;

			while(slide[piece[from]] && valid_pos(new_pos) && new_pos != to && color[new_pos] == EMPTY && distance(prev_pos, new_pos) <= 3) {
				prev_pos = new_pos;
				new_pos += offset[piece[from]][i];
			}

			if(to == new_pos) valid = true;
		}

		if(!valid) return false;
		valid = true;

		Move m = update_board(from, to);
		//if(is_check(side)) valid = true;
		//undo_move(m);
		if(valid) return true;
		return false;
	}
}


void init_board() {
	for(int i = 0; i < 64; i++) {
		piece[i] = initial_piece[i];
		color[i] = initial_color[i];
	}
	side = WHITE;
	castling = 63;
	enpassant = 0;
}

void print_board() {
	int i;
	for(i = 56; i >= 0;) {
		if(i % 8 == 0) std::cout << (i / 8) + 1 << "  ";
		std::cout << " ";

		if(color[i] == WHITE) {
			std::cout << WHITE_COLOR;
			if(piece[i] == PAWN) std::cout << 'P';
			else if(piece[i] == KNIGHT) std::cout << 'N';
			else if(piece[i] == BISHOP) std::cout << 'B';
			else if(piece[i] == ROOK) std::cout << 'R';
			else if(piece[i] == QUEEN) std::cout << 'Q';
			else if(piece[i] == KING) std::cout << 'K';
		} else if(color[i] == BLACK) {
			std::cout << WHITE_COLOR;
			if(piece[i] == PAWN) std::cout << 'p';
			else if(piece[i] == KNIGHT) std::cout << 'n';
			else if(piece[i] == BISHOP) std::cout << 'b';
			else if(piece[i] == ROOK) std::cout << 'r';
			else if(piece[i] == QUEEN) std::cout << 'q';
			else if(piece[i] == KING) std::cout << 'k';
		} else if(color[i] == EMPTY) {
			std::cout << EMPTY_COLOR << '.';
		}

		std::cout << RESET_COLOR;
		if((i + 1) % 8 == 0) {
			std::cout << '\n';
			i -= 15;
		} else {
			i++;
		}
	}

 	std::cout << '\n' << "   ";
	for(i = 0; i < 8; i++) std::cout << " " << char('a' + i);
	std::cout << "\n\n";
}
