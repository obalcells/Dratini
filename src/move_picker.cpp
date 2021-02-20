#include <vector> 
#include <cassert>
#include <algorithm>

#include "defs.h" 
#include "move_picker.h"
#include "board.h"
#include "gen.h"
#include "magicmoves.h"
#include "tt.h"
#include "bitboard.h"

// Tradeoffs:
// use sort() once or linearly scan for each move?
// early stopping in SEE? (Just whether it's good or bad)
// bad captures or quiet moves, which should go first?

static inline bool move_is_straight(const Move move) {
	return row(get_from(move)) == row(get_to(move)) || col(get_from(move)) == col(get_to(move));
}

static inline bool move_is_diagonal(const Move move) {
	return abs(row(get_from(move)) - row(get_to(move))) == abs(col(get_from(move)) - col(get_to(move)));
}

uint64_t MovePicker::get_attackers(const int to_sq, const bool attacker_side) const { 
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

int MovePicker::next_lva(const uint64_t& attacker_mask, const bool attacker_side) const {
	const int start_piece = (attacker_side == WHITE ? 0 : 6); 
	for(int piece = start_piece; piece < start_piece + 6; piece++) {
		if(attacker_mask & board->get_piece_mask(piece)) {
			return lsb(attacker_mask & board->get_piece_mask(piece));
		}
	} 
	return -1;
}

int MovePicker::fast_see(const Move move) const {
	const int to_sq = get_to(move);
	int from_sq = get_from(move), depth = 0, side = board->side;	
	int piece_at_to = board->get_piece(to_sq);
	uint64_t attacker_mask = get_attackers(to_sq, board->side) | get_attackers(to_sq, board->xside);	
	const uint64_t occ_mask = board->get_all_mask();
	std::vector<int> score(16, 0);
	score[0] = 0; // we want to force the capture at root

	while(from_sq != -1) {
		score[depth + 1] = piece_value[piece_at_to] - score[depth];
		depth++;
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

	score[0] = 10000; // we want to force the capture at the root
	score[depth + 1] = -score[depth];

	while(depth > 0) {
		// (Kind of) minimax optimization. At each node you can choose between not making the capture
		// or making the capture and assuming that the other side will play optimally after that
		score[depth] = std::max(-score[depth - 1], -score[depth + 1]);
		depth--;
	}

	return score[1];
}

// don't avoid doing stupid captures when at root
int MovePicker::slow_see(const Move move, const bool root) {
	const int piece_from = board->get_piece(get_from(move));
	const int piece_captured = board->get_piece(get_to(move));
	const int from_sq = get_from(move);
	const int to_sq = get_to(move);

	// making the capture
	// setting the 'to' square
	board->bits[piece_from] ^= mask_sq(from_sq);
	board->bits[piece_from] ^= mask_sq(to_sq);
	board->bits[piece_captured] ^= mask_sq(to_sq); 	
	board->side = board->xside;
	board->xside = !board->xside;

	const int captured_piece_value = piece_value[piece_captured];
	const int lva_square = lva(to_sq);

	int score;

	if(lva_square == -1) {
		score = captured_piece_value;
	} else {
		score = captured_piece_value - slow_see(Move(lva_square, get_to(move), CAPTURE_MOVE), false);
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

void MovePicker::sort_captures() {
	while(!compatible_move_stack.empty()) {
        Move move = compatible_move_stack.back();

		assert(get_flag(move) != QUIET_MOVE);

        const int from = get_from(move);
        const int to = get_to(move);
        const int piece = board->piece_at[from]; 
        int captured = board->piece_at[to];

        assert(get_flag(move) != QUIET_MOVE);

        if(get_flag(move) != CAPTURE_MOVE) {
            captured = PAWN;
        }

        move_stack.push_back(MoveWithScore(
            move,
            thread->capture_history[piece][to][captured] + 50000 * (get_flag(move) == QUEEN_PROMOTION)
        ));

		compatible_move_stack.pop_back();
	}

	std::sort(move_stack.begin(), move_stack.end());
} 

void MovePicker::sort_quiet() {
	while(!compatible_move_stack.empty()) {
        Move move = compatible_move_stack.back();

        const int from = get_from(move);
        const int to = get_to(move);
		const int flag = get_flag(move);

		if(flag == QUIET_MOVE && board->color_at[to] != EMPTY) {
			cerr << endl << endl;
			cerr << "We are generating a move with flag QUIET_MOVE which is actually capturing another piece" << endl;
			cerr << "Move is: " << move_to_str(move) << endl;
			cerr << "Board is:" << endl;
			board->print_board();
		}
		assert(flag != QUIET_MOVE || board->color_at[to] == EMPTY);

		if(board->color_at[from] != board->side) {
			cerr << endl << endl;
			cerr << "An invalid move is being generated" << endl;
			cerr << RED_COLOR << "Oh no! something went wrong" << RESET_COLOR << endl;
			cerr << "Move is: " << move_to_str(move) << endl;
			cerr << "Board is:" << endl;
			board->print_board();
			// we generate all the quiet moves again and output them
			compatible_move_stack.clear();
			generate_quiet(compatible_move_stack, board);
			cerr << "List of quiet moves generated is the following:" << endl;
			for(int i = 0; i < (int)compatible_move_stack.size(); i++) {
				cerr << move_to_str(compatible_move_stack[i]) << " ";
			}
			cerr << endl;
		}
		assert(board->color_at[from] == board->side);

        move_stack.push_back(MoveWithScore(
            move,
            thread->quiet_history[board->side][from][to]
        ));

		compatible_move_stack.pop_back();
	}

	std::sort(move_stack.begin(), move_stack.end());
}

Move MovePicker::next_move() {
	const Board board_before = *board;
	switch(phase) {
		case HASH:
			phase = GENERATE_CAPTURES;
			if(tt_move != NULL_MOVE && board->fast_move_valid(tt_move)) {
				return tt_move;
			}
			// assert(board_before == *board);

		case GENERATE_CAPTURES:
			// if we are in check we skip ahead to the evasions
			if(board->in_check()) {
				compatible_move_stack.clear();
				generate_evasions(compatible_move_stack, board);	
				sort_quiet();
				phase = QUIET;
				return next_move();
			} else {
				compatible_move_stack.clear();
				generate_captures(compatible_move_stack, board);
				sort_captures();
				phase = GOOD_CAPTURES;
			}

		case GOOD_CAPTURES:
			while(!move_stack.empty() && move_stack[move_stack.size() - 1].score >= 0) {
				Move move = move_stack.back().move;
				move_stack.pop_back();
				// still need to implement SEE check here
				if(move != tt_move
				&& board->fast_move_valid(move)) {
					return move;
				}
			}
			phase = FIRST_KILLER;

		case FIRST_KILLER:
			phase = SECOND_KILLER;

            if(!captures_only) {
				if(thread->killers[thread->ply][0] != NULL_MOVE
				&& thread->killers[thread->ply][0] != tt_move
				&& board->fast_move_valid(thread->killers[thread->ply][0])) {
					return thread->killers[thread->ply][0];
				}
            }

		case SECOND_KILLER:
			phase = BAD_CAPTURES;

			if(!captures_only) {
				if(thread->killers[thread->ply][1] != NULL_MOVE
				&& thread->killers[thread->ply][1] != tt_move
				&& thread->killers[thread->ply][1] != thread->killers[thread->ply][0] 
				&& board->fast_move_valid(thread->killers[thread->ply][0])) {
					return thread->killers[thread->ply][0];
				}
			}

		case BAD_CAPTURES:
			while(!move_stack.empty()) {
				Move move = move_stack.back().move;
				move_stack.pop_back();

				if(move != tt_move
				&& board->fast_move_valid(move)) {
					return move;
				}
			}

            if(captures_only) {
                phase = DONE;
                break;
            } else {
				phase = GENERATE_QUIET;
			}

		case GENERATE_QUIET:
			compatible_move_stack.clear();
			generate_quiet(compatible_move_stack, board);			
			sort_quiet();
			phase = QUIET;

		case QUIET:
			while(!move_stack.empty()) {
				Move move = move_stack.back().move;
				move_stack.pop_back();

				if(move != tt_move
				&& move != thread->killers[thread->ply][1]
				&& move != thread->killers[thread->ply][1] 
				&& board->fast_move_valid(move)) {
					return move;
				}
			}	
	}

	return NULL_MOVE;
}
