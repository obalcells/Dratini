#include <vector> 
#include <cassert>
#include <algorithm>

#include "defs.h" 
#include "move_picker.h"
#include "new_board.h"
#include "new_gen.h"
#include "magicmoves.h"
#include "new_gen.h"
#include "tt.h"
#include "bitboard.h"

// Tradeoffs:
// use sort() once or linearly scan for each move?
// early stopping in SEE? (Just whether it's good or bad)
// bad captures or quiet moves, who should go first?

static const int piece_value[12] = {
	100, 350, 350, 500, 1000, 999999,
	100, 350, 350, 500, 1000, 999999
};

static inline bool move_is_straight(const NewMove move) {
	return row(move.get_from()) == row(move.get_to()) || col(move.get_from()) == col(move.get_to());
}

static inline bool move_is_diagonal(const NewMove move) {
	return abs(row(move.get_from()) - row(move.get_to())) == abs(col(move.get_from()) - col(move.get_to()));
}

uint64_t MovePicker::get_attackers(int to_sq, bool attacker_side) const { 
    uint64_t attackers = 0;

    if(attacker_side == BLACK) {
        if((mask_sq(to_sq) & ~COL_0) && board->get_piece(to_sq + 7) == BLACK_PAWN)
            attackers |= mask_sq(to_sq + 7); 
        if((mask_sq(to_sq) & ~COL_7) && board->get_piece(to_sq + 9) == BLACK_PAWN)
            attackers |= mask_sq(to_sq + 9);
    } else {
        if((mask_sq(to_sq) & ~COL_0) && board->get_piece(to_sq - 9) == WHITE_PAWN)
            attackers |= mask_sq(to_sq - 9);
        if((mask_sq(to_sq) & ~COL_7) && board->get_piece(to_sq - 7) == WHITE_PAWN) 
            attackers |= mask_sq(to_sq - 7);
    }

    attackers |= Rmagic(to_sq, board->get_all_mask()) & 
                (board->get_rook_mask(attacker_side) | board->get_queen_mask(attacker_side));
    attackers |= Bmagic(to_sq, board->get_all_mask()) &
                (board->get_bishop_mask(attacker_side) | board->get_queen_mask(attacker_side));
    attackers |= knight_attacks[to_sq] &
                board->get_knight_mask(attacker_side);

    return attackers;
}

// we assume attacker is xside
int MovePicker::lva(int to_sq) const {
    if(board->xside == BLACK) {
        if((mask_sq(to_sq) & ~COL_0) && board->get_piece(to_sq + 7) == BLACK_PAWN)
        	return to_sq + 7;
        if((mask_sq(to_sq) & ~COL_7) && board->get_piece(to_sq + 9) == BLACK_PAWN)
        	return to_sq + 9; 
    } else {
        if((mask_sq(to_sq) & ~COL_0) && board->get_piece(to_sq - 9) == WHITE_PAWN)
        	return to_sq - 9; 
        if((mask_sq(to_sq) & ~COL_7) && board->get_piece(to_sq - 7) == WHITE_PAWN) 
        	return to_sq - 7;
    }
	if(knight_attacks[to_sq] & board->get_knight_mask(board->xside))
		return lsb(knight_attacks[to_sq] & board->get_knight_mask(board->xside));
    if(Rmagic(to_sq, board->get_all_mask()) & (board->get_rook_mask(board->xside) | board->get_queen_mask(board->xside)))
    	return lsb(Rmagic(to_sq, board->get_all_mask()) & (board->get_rook_mask(board->xside) | board->get_queen_mask(board->xside)));
    if(Bmagic(to_sq, board->get_all_mask()) & (board->get_bishop_mask(board->xside) | board->get_queen_mask(board->xside)))
    	return lsb(Bmagic(to_sq, board->get_all_mask()) & (board->get_bishop_mask(board->xside) | board->get_queen_mask(board->xside)));
    return -1;
}

int MovePicker::next_lva(const uint64_t& attacker_mask, bool attacker_side) const {
	const int start_piece = (attacker_side == WHITE ? 0 : 6); 
	for(int piece = start_piece; piece < start_piece + 6; piece++) {
		if(attacker_mask & board->get_piece_mask(piece)) {
			return lsb(attacker_mask & board->get_piece_mask(piece));
		}
	} 
	return -1;
}

int MovePicker::fast_see(const NewMove& move) {
	std::cout << BLUE_COLOR << "RUNNING FAST SEE" << RESET_COLOR << endl;
	const int to_sq = move.get_to();
	int from_sq = move.get_from(), depth = 1, side = board->side;	
	int piece_at_to = piece_value[board->get_piece(to_sq)];
	uint64_t attacker_mask = get_attackers(to_sq, board->side) | get_attackers(to_sq, board->xside);	
	const uint64_t occ_mask = board->get_all_mask();
	std::vector<int> score(16);
	score[0] = 100000; // we want to force root to capture

	while(from_sq != -1) {
		score[depth + 1] = -score[depth++] + piece_value[piece_at_to];
		// fake the move
		piece_at_to = board->get_piece(from_sq);
		attacker_mask ^= mask_sq(from_sq);
		side = !side;

		// check xrays
		if(move_is_diagonal(move)) {
			uint64_t _diagonal_mask;
			if(from_sq < to_sq) {
				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][NORTHEAST];
				else _diagonal_mask = diagonal_mask[from_sq][NORTHWEST];
			} else {
				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][SOUTHEAST];
				else _diagonal_mask = diagonal_mask[from_sq][SOUTHWEST];
			}
			attacker_mask |= Bmagic(from_sq, occ_mask) & _diagonal_mask;
		} else if(move_is_straight(move)) {
			if(row(from_sq) == row(to_sq)) {
				attacker_mask |= Rmagic(from_sq, board->get_all_mask()) & straight_mask[from_sq][(from_sq < to_sq ? EAST : WEST)];
			} else {
				attacker_mask |= Rmagic(from_sq, board->get_all_mask()) & straight_mask[from_sq][(from_sq < to_sq ? NORTH : SOUTH)];
			}
		}

		// if next_lva returns -1 it means that there are no more attackers from current attacker side 
		from_sq = next_lva(attacker_mask, side);
		// the king has put himself in check
		if(piece_at_to == WHITE_KING || piece_at_to == BLACK_KING) {
			score[depth--] = 0;
			from_sq = -1;
		}
	}

	while(--depth > 1) {
		// Minimax optimization. At each node you can choose between:
		// 1) Not making the capture
		// 2) Making the capture and assuming that the other side will play optimally
		score[depth] = std::max(-score[depth + 1], -score[depth - 1]);
	}

	if(score[0] != slow_see(move, true)) {
		std::string error_msg = "Fast and slow SEE differ for move " + move.get_str(); 
		throw(error_msg);
	}

	return score[0];
}

// don't avoid doing stupid captures when at root
int MovePicker::slow_see(const NewMove& move, bool root) {
	const int captured_piece_value = piece_value[board->get_piece(move.get_to())];
	const int lva_square = lva(move.get_to());

	if(lva_square == -1) {
		return captured_piece_value;
	}

	// making the capture
	board->bits[board->get_piece(move.get_from())] ^= mask_sq(lva_square);
	board->bits[board->get_piece(move.get_from())] ^= mask_sq(move.get_to());
	board->bits[board->get_piece(move.get_to())] ^= mask_sq(move.get_to()); 	
	board->side = board->xside;
	board->xside = !board->xside;

	const int score = captured_piece_value - slow_see(NewMove(lva_square, move.get_to(), CAPTURE_MOVE), false);

	// unmaking move 
	board->bits[board->get_piece(move.get_from())] ^= mask_sq(lva_square);
	board->bits[board->get_piece(move.get_from())] ^= mask_sq(move.get_to());
	board->bits[board->get_piece(move.get_to())] ^= mask_sq(move.get_to()); 	
	board->side = board->xside;
	board->xside = !board->xside;

	if(!root) {
		return std::max(0, score);
	}

	return score;
}

void MovePicker::sort_evasions() {
	while(!compatible_move_stack.empty()) {
		move_stack.push_back(NewMoveWithScore(compatible_move_stack.back(), 1));
		compatible_move_stack.pop_back();
	}
}

void MovePicker::sort_captures() {
	while(!compatible_move_stack.empty()) {
		BitBoard prev_board = *board;
		move_stack.push_back(NewMoveWithScore(compatible_move_stack.back(), slow_see(compatible_move_stack.back(), true)));
		assert(prev_board == *board);
		compatible_move_stack.pop_back();
	}
	std::sort(move_stack.begin(), move_stack.end());
} 

void MovePicker::sort_quiet() {
	while(!compatible_move_stack.empty()) {
		move_stack.push_back(NewMoveWithScore(compatible_move_stack.back(), 1));
		compatible_move_stack.pop_back();
	}
}

NewMove MovePicker::next_move() {
	switch(phase) {
		case HASH:
			phase = GENERATE_CAPTURES;
			Move _hash_move;
			if(false && tt.retrieve_move(board->key, _hash_move)) {
				assert(board->move_valid(hash_move));
				hash_move.bits = _hash_move;
				return hash_move;
			}

		case GENERATE_CAPTURES:
			// if we are in check we skip ahead to the evasions
			if(board->in_check()) {
				generate_evasions(compatible_move_stack, board);	
				sort_evasions();
				phase = QUIET;
				return next_move(); // recursive call
			} else {
				generate_captures(compatible_move_stack, board);
				sort_captures();
				phase = GOOD_CAPTURES;
			}

		case GOOD_CAPTURES:
			if(!move_stack.empty() && move_stack[move_stack.size() - 1].score >= 0) {
				NewMove move = move_stack.back().move;
				move_stack.pop_back();
				if(move != hash_move) {
					return move;
				}
			}
			phase = KILLERS;

		case KILLERS:
			// killer moves haven't been yet implemented 
			phase = BAD_CAPTURES;

		case BAD_CAPTURES:
			if(!move_stack.empty()) {
				NewMove move = move_stack.back().move;
				move_stack.pop_back();
				while(!move_stack.empty()) {
					if(move == hash_move
					// || move == killer_move_1
					// || move == killer_move_2
					) {
						move = move_stack.back().move;
						move_stack.pop_back();	
						continue;
					}
					return move;
				}
				phase++;
				if(move != hash_move
				// && move != killer_move_1
				// && move != killer_move_2
				) {
					return move;
				}
			}
			phase = GENERATE_QUIET;

		case GENERATE_QUIET:
			generate_quiet(compatible_move_stack, board);			
			sort_quiet();
			phase = QUIET;

		case QUIET:
			if(!move_stack.empty()) {
				NewMove move = move_stack.back().move;
				move_stack.pop_back();
				return move;
			}	
	}

	return NewMove(); // null move
}
