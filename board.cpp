#include <iostream>
#include <cassert>
#include <vector>
#include "defs.h"
#include "protos.h"
#include "data.h"

inline int distance(int pos_1, int pos_2) {
  return abs(row(pos_1) - row(pos_2)) + abs(col(pos_1) - col(pos_2));
}

inline bool valid_distance(int pos_1, int pos_2) {
  return valid_pos(pos_1) && valid_pos(pos_2) && distance(pos_1, pos_2) <= 3;
}

bool is_attacked(int pos, int attacker_side) {
	//pawns
	if(attacker_side == BLACK) {
		if(valid_distance(pos, pos + 7) && piece[pos + 7] == PAWN && color[pos + 7] == attacker_side) return true;
		if(valid_distance(pos, pos + 9) && piece[pos + 9] == PAWN && color[pos + 9] == attacker_side) return true;
	} else {
		if(valid_distance(pos, pos - 7) && piece[pos - 7] == PAWN && color[pos - 7] == attacker_side) return true;
		if(valid_distance(pos, pos - 9) && piece[pos - 9] == PAWN && color[pos - 9] == attacker_side) return true;
	}

  int i, new_pos = pos;

	//knight's offset
	for(i = 0; i < 8; i++) {
		if(offset[KNIGHT][i] == 0) break; // done
		if(!valid_distance(pos, pos + offset[KNIGHT][i])) continue;
		if(piece[pos + offset[KNIGHT][i]] == KNIGHT && color[pos + offset[KNIGHT][i]] == attacker_side) return true;
	}

	//bishop's and queen's offset
	for(int i = 0; i < 8; i++) {
    if(offset[BISHOP][i] == 0) break;
		new_pos = pos;
		while(valid_distance(new_pos, new_pos + offset[BISHOP][i]) && color[new_pos + offset[BISHOP][i]] != (attacker_side ^ 1)) {
      new_pos += offset[BISHOP][i];
      if((piece[new_pos] == BISHOP || piece[new_pos] == QUEEN) && color[new_pos] == attacker_side) return true;
      else if(color[new_pos] == attacker_side) break;
    }
  }

	//rook's and queen's offset
	for(int i = 0; i < 8; i++) {
    if(offset[ROOK][i] == 0) break;
		new_pos = pos;
    while(valid_distance(new_pos, new_pos + offset[ROOK][i]) && color[new_pos + offset[ROOK][i]] != (attacker_side ^ 1)) {
      new_pos += offset[ROOK][i];
      if((piece[new_pos] == ROOK || piece[new_pos] == QUEEN) && color[new_pos] == attacker_side) return true;
      else if(color[new_pos] == attacker_side) break;
    }
  }

  for(int delta : { 8, 9, 1, -7, -8, -9, -1, 7}) {
    if(valid_pos(pos + delta) && piece[pos + delta] == KING && color[pos + delta] == attacker_side) return true;
  }

	return false;
}


bool in_check(int _side) {
	for(int pos = 0; pos < 64; pos++) {
		if(piece[pos] == KING && color[pos] == _side) {
		  return is_attacked(pos, _side ^ 1);
		}
	}
}

// we assume that the move is valid
Move make_move(int from, int to, int promotion_piece = QUEEN) {
  	assert(piece[from] != EMPTY);

  	int prev_enpassant = enpassant; // store previous flags
  	int prev_castling = castling;
	int captured = EMPTY;
	enpassant = 0; //reset the enpassant flag

	if(piece[from] == PAWN && piece[to] == EMPTY && col(from) != col(to)) {
		// eat enpassant
		int adjacent = row(from) * 8 + col(to);
		assert(piece[adjacent] == PAWN);
		assert(color[adjacent] != color[from]);

		piece[adjacent] = EMPTY;
		color[adjacent] = EMPTY;
		piece[to] = piece[from];
		color[to] = color[from];
		piece[from] = EMPTY;
		color[from] = EMPTY;

		side ^= 1;
		xside ^= 1;
		return Move({ from, to, EMPTY, prev_castling, prev_enpassant });

	} else if(piece[from] == PAWN && (row(to) == 0 || row(to) == 7)) {
    	// promote a pawn
		if(col(to) != col(from)) {
			assert(abs(col(to) - col(from)) == 1);
			assert(color[from] != color[to]);
		}

		captured = piece[to];
		piece[to] = promotion_piece;
		color[to] = side;
		piece[from] = EMPTY;
		color[from] = EMPTY;

		side ^= 1;
		xside ^= 1;
		return Move({ from, to, captured, prev_castling, prev_enpassant, true });

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
			if(side == WHITE && (castling & 4)) castling ^= 4;
			else if(side == BLACK && (castling & 32)) castling ^= 32;

		} else if(piece[from] == ROOK) {
			// disable castling flag for this rook
			if(side == WHITE && from == 0 && (castling << 1)) castling ^= 1;
			else if(side == WHITE && from == 7 && (castling << 2)) castling ^= 2;
			else if(side == BLACK && from == 54 && (castling << 8)) castling ^= 8;
			else if(side == BLACK && from == 63 && (castling << 16)) castling ^= 16;

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

	side ^= 1;
	xside ^= 1;
	return Move({ from, to, captured, prev_castling, prev_enpassant });
}

void take_back(Move m) {
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
    else assert(false); // if we get here, the move was invalid

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
    if(m.captured == EMPTY) color[m.to] = EMPTY;
    else color[m.to] = (color[m.from] == WHITE ? BLACK : WHITE);
	}

  castling = m.castling;
  enpassant = m.enpassant;
  side ^= 1;
  xside ^= 1;
}

int move_valid(int from, int to) {
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
		int rook_pos = -1;
		if(side == WHITE && from == 4 && to == 2 && (castling & 1) && (castling & 4)) rook_pos = 0;
		else if(side == WHITE && from == 4 && to == 6 && (castling & 2) && (castling & 4)) rook_pos = 7;
		else if(side == BLACK && from == 60 && to == 58 && (castling & 8) && (castling & 32)) rook_pos = 56;
		else if(side == BLACK && from == 60 && to == 62 && (castling & 16) && (castling & 32)) rook_pos = 63;
		else return 5;

		int pos = from, direction = (to > from ? 1 : -1);
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
			int adjacent = row(from) * 8 + col(to);
			if(piece[to] != EMPTY || color[adjacent] != xside) return 10;
			else if(enpassant != col(to)) return 11;
		}
		else if(delta_row == 1 && delta_col == 0 && piece[to] == EMPTY) { /* fine */ }
		else if(delta_row == 2 && delta_col == 0 && piece[to] == EMPTY && piece[from + (side == WHITE ? 8 : -8)] == EMPTY) { /* fine too */ }
		else if(delta_row == 1 && delta_col == 1 && color[to] != EMPTY && color[to] == xside) { /* fine too */ }
		else return 13; /* not fine */

	} else {
		// any other movement

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
		if(!valid) return 14;
	}

	Move m = make_move(from, to);
	bool valid = true;
  if(in_check(xside)) valid = false; // remember that side gets flipped
	take_back(m);

	if(valid) return 0; // move is valid
  else return 15;
}

void add_move(int from, int to) {
  assert(from >= 0 && to >= 0);

  Move m = make_move(from, to);

  if(!in_check(xside)) {
    take_back(m);

    int error_code = move_valid(from, to);
    if(error_code != 0) {
      std::cout << "Generated move by " << piece[from] << " " << from << " " << to << " is invalid, error = " << error_code << "\n";
      print_board();
      assert(error_code == 0);
    }

    move_stack.push_back(m);

  	} else {
	  take_back(m);
  	}
}

void generate_capture_moves() {
  for(int pos = 0; pos < 64; pos++) if(color[pos] == side) {
    if(piece[pos] == PAWN) {
      // diagonal-capture
      int one_forward = (side == WHITE ? pos + 8 : pos - 8);

			if(valid_distance(pos, one_forward - 1) && color[one_forward - 1] == xside)
				add_move(pos, one_forward - 1);

			if(valid_distance(pos, one_forward + 1) && color[one_forward + 1] == xside)
				add_move(pos, one_forward + 1);

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
      for(int i = 0; i < 8; i++) {
        if(offset[piece[pos]][i] == 0) break;
        int new_pos = pos + offset[piece[pos]][i];

        if(!slide[piece[pos]]) {
          if(valid_distance(pos, new_pos) && color[new_pos] == xside) {
            add_move(pos, new_pos);
          }
        } else {
          int prev_pos = pos;
          while(valid_distance(prev_pos, new_pos) && color[new_pos] != side) {
            if(color[new_pos] == xside) {
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
}

void generate_moves() {
	for(int pos = 0; pos < 64; pos++) if(color[pos] == side) {
		if(piece[pos] == PAWN) {
			// one forward
			int one_forward = (side == WHITE ? pos + 8 : pos - 8);
			assert(one_forward >= 0 && one_forward <= 63);
			int captured = EMPTY;
			if(row(one_forward) == 0 || row(one_forward) == 7) captured = PAWN;
			if(color[one_forward] == EMPTY) add_move(pos, one_forward);

			// two forward
			if((side == WHITE && row(pos) == 1) || (side == BLACK && row(pos) == 7)) {
				int two_forward = (side == WHITE ? pos + 16 : pos - 16);
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
				if(side == WHITE && castling & 4) {
					if(castling & 1 && move_valid(4, 2) == 0) add_move(4, 2);
					if(castling & 2 && move_valid(4, 6) == 0) add_move(4, 6);
					if(castling & 8 && move_valid(60, 58) == 0) add_move(60, 58);
					if(castling & 16 && move_valid(60, 62) == 0) add_move(60, 62);
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
}

void init_board() {
	for(int i = 0; i < 64; i++) {
		piece[i] = initial_piece[i];
		color[i] = initial_color[i];
	}
	side = WHITE;
	xside = BLACK;
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
  std::cout << "\n\n"; //std::cout << "\n Parameters: " << side << " " << xside << " " << castling << " " << enpassant << "\n\n";
}

void save_snapshot(std::string snapshot_name) {
  freopen((snapshot_name + ".snapshot").c_str(), "w", stdout);

  std::cout << side << endl;
  std::cout << castling << endl;
  std::cout << enpassant << endl;

  for(int i = 0; i < 64; i++) {
    std::cout << color[i] << " " << piece[i] << endl;
  }

  freopen("/dev/tty", "w", stdout);
  std::cout << "Snapshot made!" << endl; // this should be printed in console
}

void load_snapshot(std::string snapshot_name) {
  freopen((snapshot_name + ".snapshot").c_str(), "r", stdin);

  std::cin >> side;
  xside = side ^ 1;
  std::cin >> castling;
  std::cin >> enpassant;

  for(int i = 0; i < 64; i++) {
    std::cin >> color[i];
    std::cin >> piece[i];
  }

  freopen("/dev/tty", "r", stdin);
  std::cout << "Snapshot loaded!" << endl;
}


