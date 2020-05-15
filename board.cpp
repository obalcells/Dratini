#include <iostream>
#include <cassert>
#include "data.h"
#include "protos.h"

void init_board() {
	for(int i = 0; i < 64; i++) {
		piece[i] = initial_piece[i];
		color[i] = initial_color[i];
	}
	side = WHITE;
	flags = 15;
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

int distance(int pos_1, int pos_2) {
	return abs(row(pos_1) - row(pos_2)) + abs(col(pos_1) - col(pos_2));
}

bool valid_pos(int pos) {
	return (pos >= 0 && pos < 64) ? true : false;
}


bool is_attacked(int pos, int s) {
	int i;

	//pawns
	if(side == 1) {
		if(pos + 7 < 64 && piece[pos + 7] == PAWN && color[pos + 7] == s) return true; 
		if(pos + 9 < 64 && piece[pos + 9] == PAWN && color[pos + 9] == s) return true;
	} else {
		if(pos - 7 >= 0 && piece[pos - 7] == PAWN && color[pos - 7] == s) return true; 
		if(pos - 9 >= 0 && piece[pos - 9] == PAWN && color[pos - 9] == s) return true;
	}
	
	int new_pos = pos;
	int prev_pos;
	//knight's offset
	for(i = 0; i < 8; i++) {
		if(offset[KNIGHT][i] == 0) break;

		new_pos = pos + offset[KNIGHT][i];
		if(!valid_pos(new_pos) || distance(pos, new_pos) > 3) continue;

		if(piece[new_pos] == KNIGHT && color[new_pos] == s) return true; 
	}	

	//bishop's offset
	for(i = 0; i < 8; i++) {
		if(offset[BISHOP][i] == 0) break;

		new_pos = pos + offset[BISHOP][i]; prev_pos = pos;
		if(!valid_pos(new_pos) || distance(prev_pos, new_pos) != 2) continue;

		while(valid_pos(new_pos) && color[new_pos] == EMPTY && distance(prev_pos, new_pos) == 2) {
			prev_pos = new_pos;
			new_pos += offset[BISHOP][i];
		}

		if(valid_pos(new_pos) && (piece[new_pos] == BISHOP || piece[new_pos] == QUEEN) && color[new_pos] == s) return true;
	}
	
	//rook's offset
	for(i = 0; i < 8; i++) {
		if(offset[ROOK][i] == 0) break;		
		new_pos = pos + offset[ROOK][i]; prev_pos = pos;
		if(!valid_pos(new_pos) || distance(prev_pos, new_pos) != 1) continue;
		
		while(valid_pos(new_pos) && color[new_pos] == EMPTY && distance(prev_pos, new_pos) == 1) {
			prev_pos = new_pos;
			new_pos += offset[ROOK][i];
		}

		if(valid_pos(new_pos) && (piece[new_pos] == ROOK || piece[new_pos] == QUEEN) && color[new_pos] == s) return true;
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

void undo_move(Move m) {

	assert(piece[m.from] == EMPTY);
	assert(color[m.from] == EMPTY);
	assert(piece[m.to] != EMPTY);
	assert(color[m.to] != EMPTY);

	piece[m.from] = piece[m.to];
	color[m.from] = color[m.to]; 
	piece[m.to] = m.captured; 
	if(m.captured != EMPTY) color[m.to] = (color[m.to] == BLACK ? WHITE : BLACK); 
	
	//promotions
	//enpassant
	//castling

	if(piece[m.to] == PAWN && abs(col(m.from) - col(m.to)) == 1 && m.captured == EMPTY) {
		//ep
		piece[m.from] = PAWN;
		color[m.from] = color[m.to];
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;
		piece[from + (side == 0 ? 8 : -8)] = PAWN;
		color[from + (side == 0 ? 8 : -8)] = (color[m.from] == BLACK ? WHITE : BLACK);

	} else if(m.promotion == true) {
		//promotion
		assert(piece[m.to] != EMPTY);

		piece[m.from] = PAWN;
		color[m.from] = color[m.to];
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;
		
	} else if(piece[m.to] == KING && row(m.from) == row(m.to) && abs(col(m.from) - col(m.to)) == 2) {
		//castling
		piece[m.from] = KING;	
		color[m.from] = color[m.to];
		piece[m.to] = EMPTY;
		color[m.to] = EMPTY;
		
		if(m.to < m.from) { //castling to the left
			piece[m.to + 1] = EMPTY;
			color[m.to + 1] = EMPTY;
			if(color[m.from] == WHITE) {
				piece[0] = ROOK;
				color[0] = WHITE;
				flags |= (1 << 2);
				flags |= (1 << 0); 
			} else {
				piece[54] = ROOK;
				color[0] = BLACK;
				flags |= (1 << 5);
				flags |= (1 << 3); //i dont know if this is right
			}
		} else {
			piece[m.to - 1] = EMPTY;
			color[m.to - 1] = EMPTY;
			if(color[m.from] == WHITE) {
				piece[7] = ROOK;
				color[7] = WHITE;
				flags |= (1 << 2);
				flags |= (1 << 1);
			} else {
				piece[63] = ROOK;
				color[63] = BLACK;
				flags |= (1 << 5);
				flags |= (1 << 4);
			}
		}
	} else if() {

	}

}

//alters the global board state with a new move
//it is known that the move is valid 
Move make_move(int from, int to, int promotion_piece=QUEEN) {

	int captured = EMPTY;
	bool promotion = false;

	if(piece[from] == PAWN && piece[to] == EMPTY && row(from) != row(to)) {
		//ENPASSANT
		piece[from] = EMPTY;
		color[from] = EMPTY;
		piece[to] = PAWN;
		color[to] = side;
		piece[from + (side == WHITE ? 8 : -8)] = EMPTY;
		color[from + (side == BLACK ? 8 : -8)] = EMPTY;

	} else if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		//CASTLING
		piece[from] = EMPTY;
		color[from] = EMPTY;
		piece[to] = KING;
		color[to] = side;
		if(to < from) { //castling to the left
			piece[row(from) * 8] = EMPTY;
			color[row(from) * 8] = EMPTY;
			piece[to + 1] = ROOK;
			color[to + 1] = side;
		} else {
			piece[row(from) * 8 + 7] = EMPTY;
			color[row(from) * 8 + 7] = EMPTY;
			piece[to - 1] = ROOK;
			color[to - 1] = side;
		}

		//disable the castling flag for this direction and the global for the side
		if(side == 0) {
			assert(flags & (1 << 2));
			assert(flags & (1 << (to < from ? 0 : 1)));
			flags ^= (1 << 2); 
			flags ^= (1 << (to < from ? 0 : 1));
		} else {
			assert(flags & (1 << 5));
			assert(flags & (1 << (to < from ? 0 : 1)));
			flags ^= (1 << 5);
			flags ^= (1 << (to < from ? 3 : 4));
		}

	} else if(piece[from] == PAWN && (row(to) == 0 || row(to) == 7)) {
		//PROMOTION
		promotion = true;
		piece[from] = EMPTY;
		color[from] = EMPTY;
		piece[to] = promotion_piece; 
		color[to] = side;
	} else {
		//ANYTHING ELSE
		if(piece[from] == KING) {
			//disable castling flag for the king
			if(side == 0 && (flags & (1 << 2))) flags ^= (1 << 2);
			else if(side == 1 && (flags & (1 << 5))) flags ^= (1 << 5);
		} else if(piece[from] == ROOK) {
			//disable castling flag for this rook
			if(side == 0 && from == 0 && (flags << (1 << 0))) flags ^= (1 << 0);
			else if(side == 0 && from == 7 && (flags << (1 << 1))) flags ^= (1 << 1);
			else if(side == 1 && from == 54 && (flags << (1 << 3))) flags ^= (1 << 3);
			else if(side == 1 && from == 63 && (flags << (1 << 4))) flags ^= (1 << 4);
		} else if(piece[from] == PAWN && abs(row(from) - row(to)) == 2) {
			//activate enpassant flag	
			if(side == 0) {
				flags |= (1 << (6 + col(from)));
			} else {
				flags |= (1 << (14 + col(from)));
			}
		}

		captured = piece[to];
		piece[to] = piece[from];
		color[to] = side;
		piece[from] = EMPTY;
		color[from] = EMPTY;
	}

}

void undo_move(Move m) {
	assert(piece[to] != EMPTY);
	assert(color[to] != EMPTY);
	assert(piece[from] == EMPTY);
	assert(color[from] == EMPTY);

	piece[from] = piece[to];
	color[from] = color[to];
	if(captured != EMPTY) color[to] = (side == 0 ? 1 : 0);
	else color[to] = EMPTY;
	piece[to] = captured;
}

bool is_move_valid(int from, int to) {

	//piece has to move
	if(from == to) return false;
	//the origin square can't be empty
	if(piece[from] == EMPTY) return false;
	//only friendly pieces can be moved
	if(side != color[from]) return false;
	//we can't move to squares occupied by friendly pieces
	if(color[from] == color[to]) return false;

	if(piece[from] == KING && row(from) == row(to) && abs(col(from) - col(to)) == 2) {
		//CASTLING ONLY

		//detect if castling is valid, following conditions must hold:
		// 1) King hasn't moved
		// 2) Rook of specific side hasn't moved
		// 3) King isn't in check
		// 4) There are no pieces in between
		// 5) The king can't be checked on the squares in between

		int castling_type = (to > from ? 1 : 0); //0 is left and 1 is right
		if(!(flags & (1 << (side * 2 + castling_type)))) return false; //there is a castling flag in the game's state
		if(is_attacked(from, side ^ 1)) return false; //king can't be under check

		if(castling_type == 0) {
			if(is_attacked(from - 1, side ^ 1)) return false;	
			if(piece[from - 1] != EMPTY) return false;
			if(is_attacked(from - 2, side ^ 1)) return false;
			if(piece[from - 2] != EMPTY) return false;
		} else {
			if(is_attacked(from + 1, side ^ 1)) return false;
			else if(piece[from + 1] != EMPTY) return false;
			if(is_attacked(from + 2, side ^ 1)) return false;
			else if(piece[from + 2] != EMPTY) return false;
		}

		return true;

	} else if(piece[from] == PAWN) {
		//PAWN MOVEMENTS

		//remember: side = 0 -> WHITE, side = 1 -> BLACK
		if(side == 0 && to < from) return false; 
		else if(side == 1 && to > from) return false;

		//difference in rows and cols
		int dr = abs(row(to) - row(from));
		int dc = abs(col(to) - col(from));

		if(dr == 1 && dc == 1 && piece[to] == EMPTY && piece[row(from) * 8 + col(to)] == PAWN && color[row(from) * 8 + col(to)] == EMPTY) {
			//enpassant flag-checking
			if(side == 1 && !(flags & (1 << (6 + col(to))))) return false;
			else if(side == 0 && !(flags & (1 << (14 + col(to))))) return false;
		}
		else if(dr == 1 && dc == 0 && piece[to] == EMPTY) {}
		else if(dr == 2 && dc == 0 && piece[to] == EMPTY && piece[from + (side == 0 ? 8 : -8)] == EMPTY) {}
		else if(dr == 1 && dc == 1 && color[to] != EMPTY && color[to] != color[from]) {}
		else return false;

		Move m = update_board(from, to);
		bool valid = true;
		if(is_check(side)) valid = false;
		undo_move(m);
		if(valid) return true;
		else return false;

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
		if(is_check(side)) valid = true;
		undo_move(m);
		if(valid) return true;
		return false;
	}
}


