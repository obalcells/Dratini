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
// bad captures or quiet moves, which should go first?

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

// we assume attacker is side
// least valuable attacker - we assume that attacker side is 'side'
int MovePicker::lva(int to_sq) const {
    if(board->side == BLACK) {
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
	if(knight_attacks[to_sq] & board->get_knight_mask(board->side))
		return lsb(knight_attacks[to_sq] & board->get_knight_mask(board->side));
    if(Rmagic(to_sq, board->get_all_mask()) & (board->get_rook_mask(board->side) | board->get_queen_mask(board->side)))
    	return lsb(Rmagic(to_sq, board->get_all_mask()) & (board->get_rook_mask(board->side) | board->get_queen_mask(board->side)));
    if(Bmagic(to_sq, board->get_all_mask()) & (board->get_bishop_mask(board->side) | board->get_queen_mask(board->side)))
    	return lsb(Bmagic(to_sq, board->get_all_mask()) & (board->get_bishop_mask(board->side) | board->get_queen_mask(board->side)));
    return -1;
}

int MovePicker::next_lva(const uint64_t& attacker_mask, bool attacker_side) const {
	// cout << "Running next_lva with the following attacker mask" << endl;
	// board->print_bitboard(attacker_mask);
	const int start_piece = (attacker_side == WHITE ? 0 : 6); 
	for(int piece = start_piece; piece < start_piece + 6; piece++) {
		if(attacker_mask & board->get_piece_mask(piece)) {
			// cout << pos_to_str(lsb(attacker_mask & board->get_piece_mask(piece))) << endl; 
			// cout << BLUE_COLOR << "------------------------" << RESET_COLOR << endl;
			return lsb(attacker_mask & board->get_piece_mask(piece));
		}
	} 
	// cout << -1 << endl;
	// cout << BLUE_COLOR << "------------------------" << RESET_COLOR << endl;
	return -1;
}

int MovePicker::fast_see(const NewMove& move) {
	// cout << BLUE_COLOR << "RUNNING FAST SEE" << RESET_COLOR << endl;
	const int to_sq = move.get_to();
	int from_sq = move.get_from(), depth = 0, side = board->side;	
	int piece_at_to = board->get_piece(to_sq);
	uint64_t attacker_mask = get_attackers(to_sq, board->side) | get_attackers(to_sq, board->xside);	

	// uint64_t white_diagonal_movers = board->get_queen_mask(WHITE) | board->get_bishop_mask(WHITE);
	// uint64_t black_diagonal_movers = board->get_queen_mask(BLACK) | board->get_bishop_mask(BLACK);
	// uint64_t white_straight_movers = board->get_queen_mask(WHITE) | board->get_rook_mask(WHITE);
	// uint64_t black_straight_movers = board->get_queen_mask(BLACK) | board->get_rook_mask(BLACK);

	// cout << "Attacker mask is:" << endl;
	// board->print_bitboard(attacker_mask);

	const uint64_t occ_mask = board->get_all_mask();
	std::vector<int> score(16, 0);
	score[0] = 0; // we want to force the capture at root

	while(from_sq != -1) {
		score[depth + 1] = piece_value[piece_at_to] - score[depth];
		depth++;
		// cout << BLUE_COLOR << score[depth - 1] << " " << piece_value[piece_at_to] << " " << piece_at_to << RESET_COLOR << endl;
		// cout << MAGENTA_COLOR << "Score at depth " << depth << " is " << score[depth] << RESET_COLOR << endl;
		// fake the move
		piece_at_to = board->get_piece(from_sq);
		attacker_mask ^= mask_sq(from_sq);
		side = !side;

		// check xrays
		if(move_is_diagonal(move)) {
			uint64_t _diagonal_mask;
			if(from_sq > to_sq) { // north
				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][NORTHEAST];
				else _diagonal_mask = diagonal_mask[from_sq][NORTHWEST];
			} else { // south
				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][SOUTHEAST];
				else _diagonal_mask = diagonal_mask[from_sq][SOUTHWEST];
			}
			// cout << "Move is " << move_to_str(Move(from_sq, to_sq)) << endl;
			// cout << "And diagonal mask is:" << endl;
			// board->print_bitboard(_diagonal_mask);
			attacker_mask |= Bmagic(from_sq, occ_mask) & _diagonal_mask
			& (board->get_queen_mask(side) | board->get_bishop_mask(side) | board->get_queen_mask(!side) | board->get_bishop_mask(!side)); 
		} else if(move_is_straight(move)) {
			if(row(from_sq) == row(to_sq)) {
				attacker_mask |= Rmagic(from_sq, board->get_all_mask()) & straight_mask[from_sq][from_sq < to_sq ? EAST : WEST]
				& (board->get_queen_mask(side) | board->get_rook_mask(side) | board->get_queen_mask(!side) | board->get_rook_mask(!side));
			} else {
				attacker_mask |= Rmagic(from_sq, board->get_all_mask()) & straight_mask[from_sq][from_sq < to_sq ? SOUTH : NORTH]
				& (board->get_queen_mask(side) | board->get_rook_mask(side) | board->get_queen_mask(!side) | board->get_rook_mask(!side));
			}
		}

		// if next_lva returns -1 it means that there are no more attackers from current attacker side 
		from_sq = next_lva(attacker_mask, side);
		// the king has put himself in check
		if(piece_at_to == WHITE_KING || piece_at_to == BLACK_KING) {
			depth--;
			from_sq = -1;
		}
	}

	score[0] = 10000;
	score[depth + 1] = -score[depth];
	// cout << "Score at biggest depth is " << score[depth] << endl;

	while(depth > 0) {
		// Minimax optimization. At each node you can choose between:
		// 1) Not making the capture
		// 2) Making the capture and assuming that the other side will play optimally after your capture
		score[depth] = std::max(-score[depth - 1], -score[depth + 1]);
		depth--;
	}

	return score[1];
}

// don't avoid doing stupid captures when at root
int MovePicker::slow_see(const NewMove& move, bool root) {
	// if(root) {
	// 	cout << "At root" << endl;
	// }
	// cout << "Board now is:" << endl;
	// board->print_board();
	// cout << RESET_COLOR;

	const int piece_from = board->get_piece(move.get_from());
	const int piece_captured = board->get_piece(move.get_to());
	const int from_sq = move.get_from();
	const int to_sq = move.get_to();

	// making the capture
	// setting the 'to' square
	board->bits[piece_from] ^= mask_sq(from_sq);
	board->bits[piece_from] ^= mask_sq(to_sq);
	board->bits[piece_captured] ^= mask_sq(to_sq); 	
	board->side = board->xside;
	board->xside = !board->xside;

	const int captured_piece_value = piece_value[piece_captured];
	const int lva_square = lva(to_sq);

	// if(root) {
	// 	cout << BLUE_COLOR << "At SEE, root = " << (root ? "yes" : "no") << " and move made is " << move.get_str() << RESET_COLOR << endl;	
	// } else {
	// 	cout << "At SEE, root = " << (root ? "yes" : "no") << " and move made is " << move.get_str() << endl;	
	// }
	// board->print_board();
	// cout << "LVA now is " << lva_square << endl;

	int score;

	if(lva_square == -1) {
		// cout << "LVA square is -1, just returning" << endl;
		score = captured_piece_value;
	} else {
		// cout << "Running another SEE" << endl;
		score = captured_piece_value - slow_see(NewMove(lva_square, move.get_to(), CAPTURE_MOVE), false);
	}

	// unmaking move 
	// clearing the 'to' square
	board->bits[piece_from] ^= mask_sq(from_sq);
	board->bits[piece_from] ^= mask_sq(to_sq);
	board->bits[piece_captured] ^= mask_sq(to_sq); 	
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
		int s1 = slow_see(compatible_move_stack.back());
		int s2 = fast_see(compatible_move_stack.back());
		cout << "Move is " << compatible_move_stack.back().get_str() << endl;
		cout << "And board is:" << endl;
		board->print_board();
		// cout << "Board data is:";
		// board->print_bitboard_data();
		if(s1 != s2) {
			cout << RED_COLOR << "Scores don't match" << RESET_COLOR << endl;
			cout << "Slow is " << s1 << " and fast is " << s2 << endl;
			assert(s1 == s2);
		} else {
			cout << GREEN_COLOR << s1 << RESET_COLOR << endl;
		}
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
				return next_move();
			} else {
				generate_captures(compatible_move_stack, board);
				sort_captures();
				phase = GOOD_CAPTURES;
			}

		case GOOD_CAPTURES:
			while(!move_stack.empty() && move_stack[move_stack.size() - 1].score >= 0) {
				NewMove move = move_stack.back().move;
				move_stack.pop_back();
				if(move != hash_move && board->fast_move_valid(move)) {
					return move;
				}
			}
			phase = KILLERS;

		case KILLERS:
			// killer moves haven't been yet implemented 
			phase = BAD_CAPTURES;

		case BAD_CAPTURES:
			while(!move_stack.empty()) {
				NewMove move = move_stack.back().move;
				move_stack.pop_back();
				if(move != hash_move && board->fast_move_valid(move)) {
					return move;
				}
			}
			phase = GENERATE_QUIET;

		case GENERATE_QUIET:
			generate_quiet(compatible_move_stack, board);			
			sort_quiet();
			phase = QUIET;

		case QUIET:
			while(!move_stack.empty()) {
				NewMove move = move_stack.back().move;
				move_stack.pop_back();
				if(move != hash_move && board->fast_move_valid(move)) {
					return move;
				}
			}	
	}
	return NewMove(); // null move
}
