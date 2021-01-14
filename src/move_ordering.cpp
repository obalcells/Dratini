#include <vector> 
#include "defs.h" 
#include "new_board.h"
#include "new_gen.h"

// this won't work!!!
// static std::vector<NewMove> move_stack; 
static int phase;
static NewMove hash_move; 
static NewMove move; // name collision line 21

enum phases {
	HASH,
	GENERATE_CAPTURES,
	GOOD_CAPTURES,
	KILLERS,
	BAD_CAPTURES,
	GENERATE_QUIET,
	QUIET	
}

// we assume attacker is xside
int lva(int to_sq, const BitBoard& board) {
    if(board.xside == BLACK) {
        if((mask_sq(sq) & ~COL_0) && board.get_piece(sq + 7) == BLACK_PAWN)
        	return sq + 7;
        if((mask_sq(sq) & ~COL_7) && board.get_piece(sq + 9) == BLACK_PAWN)
        	return sq + 9; 
    } else {
        if((mask_sq(sq) & ~COL_0) && board.get_piece(sq - 9) == WHITE_PAWN)
        	return sq - 9; 
        if((mask_sq(sq) & ~COL_7) && board.get_piece(sq - 7) == WHITE_PAWN) 
        	return sq - 7;
    }
    uint64_t attackers = 0;
    attackers = knight_attacks[sq] & board.get_knight_mask(attacker_side);
    if(attackers) {
    	return lsb(attackers);
    }
    attackers = Rmagic(sq, board.get_all_mask()) & 
                (board.get_rook_mask(attacker_side) | board.get_queen_mask(attacker_side));
    if(attackers) {
    	return lsb(attackers);
    }
    attackers = Bmagic(sq, board.get_all_mask()) &
                (board.get_bishop_mask(attacker_side) | board.get_queen_mask(attacker_side));
    if(attackers) {
    	return lsb(attackers);
    }
    return -1;
}

int next_lva(const uint64_t& attacker_mask, bool attacker_side, const BitBoard& board) {
	const int start_piece = (attacker_side == WHITE ? 0 : 6); 
	for(int piece = start_piece, piece < start_piece + 6; piece++) {
		if(attacker_mask & board.get_piece_mask(piece))	{
			return lsb(attacker_mask & board.get_piece_mask(piece));
		}
	} 
	return -1;
}

inline bool move_is_diagonal() {
	return abs(col(from_sq) - col(to_sq)) == abs(row(from_sq) - row(to_sq));
}

inline bool move_is_straight(int from_sq, int to_sq) {
	return col(from_sq) == col(to_sq) || row(from_sq) == row(to_sq);
}

// CREATE STRAIGHT AND DIAGONAL MASKS!!!
void check_xrays(uint64_t& attacker_mask, const uint64_t occ, const int from_sq, const int to_sq) {
	if(move_is_diagonal(from_sq, to_sq)) {
		attacker_mask |= Bmagic(from_sq, occ) &
		diagonal_mask[from_sq][(from_sq < to_sq ? NORTH : SOUTH) + (col(from_sq) < col(to_sq) ? EAST : WEST)]
	} else if(move_is_straight(from_sq, to_sq)) {
		if(row(from_sq) == row(to_sq)) {
			attacker_mask |= Rmagic(from_sq, occ) & straight_mask[from_sq][(from_sq < to_sq ? EAST : WEST)];
		} else {
			attacker_mask |= Rmagic(from_sq, occ) & straight_mask[from_sq][(from_sq < to_sq ? NORTH : SOUTH)];
		}
	}
}

int fast_see(const NewMove& move, const BitBoard& board) {
	const int to_sq = move.get_to();
	int from_sq = move.get_from(), depth = 1, side = board.side;	
	int piece_at_to = piece_value[board.get_piece(to_sq)];
	uint64_t attacker_mask = get_attackers(to_sq, board.side, board) | get_attackers(to_sq, board.xside, board);	
	std::vector<int> score(16);
	score[0] = 100000; // we want to force root to capture

	while(from_sq != -1) {
		score[depth + 1] = -score[depth++] + piece_value[piece_at_to];
		// fake the move
		piece_at_to = move.get_piece(from_sq);
		attacker_mask ^= mask_sq(from_sq);
		side = !side;
		attacker_mask |= check_xrays(from_sq, to_sq);
		// if next_lva returns -1 it means that there are no more attackers from current attacker side 
		from_sq = next_lva(attacker_mask, side);
		// the king has put himself in check
		if(piece_at_to == WHITE_KING || piece_at_to == BLACK_KING) {
			score[depth--] = 0;
			from_sq = -1;
		}
	}

	while(--depth > 1) {
		// minimax optimization
		// at each node you can choose between
		// 1) Not making the capture
		// 2) Making the capture and assuming that the other side will play optimally
		score[depth] = std::max(-score[depth + 1], -score[depth - 1]);
	}

	return score[1];
}

// don't avoid doing stupid captures when at root?
int slow_see(const NewMove& move, const BitBoard& board, bool root = true) {
	const int captured_piece_value = piece_value[move.get_to()];
	const int lva_square = lva(move.get_to(), board);
	if(lva_square == -1) {
		return captured_piece_value;
	}
	board.make_capture(move);
	const int score = captured_piece_value - slow_see(NewMove(lva_square, move.get_to(), CAPTURE_MOVE), false);
	board.unmake_capture(move);
	if(!root) {
		return std::max(0, score);
	}
	return score;
}

void init_move_generator() {
	move_stack.clear();
	phase = HASH;
}

NewMove next_move(const BitBoard& board) {
	switch(phase) {
		case HASH:
			phase++;
			if(tt.retrieve_move(board.key, hash_move)) {
				return hash_move;
			}

		case GENERATE_CAPTURES:
			if(board.in_check()) {
				generate_evasions();	
				// sort the evasions based on history
				phase = QUIET;
				return next_move(board); // recursive call
			} else {
				generate_captures(move_stack, board);
				// sort captures
			}
			phase++;

		case GOOD_CAPTURES:
			if(!move_stack.empty() && move_stack[i].see_score >= 0) {
				NewMove move = move_stack.back();
				move_stack.pop_back();
				return move;
			}
			phase++;

		case KILLERS:
			// get the moves from somewhere and use them
			phase++;

		case BAD_CAPTURES:
			if(move_stack.empty()) {
				phase++;
			} else {
				NewMove move = move_stack.back();
				move_stack.pop_back();
				while(!move_stack.empty()) {
					if(move == hash_move
					|| move == killer_move_1
					|| move == killer_move_2) {
						move = move_stack.back();
						move_stack.pop_back();	
						continue;
					}
					return move;
				}
				phase++;
				if(move != hash_move
				&& move != killer_move_1
				&& move != killer_move_2) {
					return move;
				}
			}

		case GENERATE_QUIET:
			generate_quiet();			
			// sort the quiet moves based on history
			phase++;

		case QUIET:
			if(!move_stack.empty()) {
				NewMove move = move_stack.back();
				move_stack.pop_back();
				return move;
			}	
	}
	return NewMove(); // null move
}




