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

#define get_side_mask(_side) (_side == WHITE ? \
	(board->bits[WHITE_PAWN] | board->bits[WHITE_KNIGHT] | board->bits[WHITE_BISHOP] | board->bits[WHITE_ROOK] | board->bits[WHITE_QUEEN] | board->bits[WHITE_KING]) : \
	(board->bits[BLACK_PAWN] | board->bits[BLACK_KNIGHT] | board->bits[BLACK_BISHOP] | board->bits[BLACK_ROOK] | board->bits[BLACK_QUEEN] | board->bits[BLACK_KING]))
#define get_piece_mask(piece) board->bits[piece]
#define get_all_mask() board->occ_mask
#define get_pawn_mask(_side) (_side == WHITE ? board->bits[WHITE_PAWN] : board->bits[BLACK_PAWN])
#define get_knight_mask(_side) (_side == WHITE ? board->bits[WHITE_KNIGHT] : board->bits[BLACK_KNIGHT])
#define get_bishop_mask(_side) (_side == WHITE ? board->bits[WHITE_BISHOP] : board->bits[BLACK_BISHOP])
#define get_rook_mask(_side) (_side == WHITE ? board->bits[WHITE_ROOK] : board->bits[BLACK_ROOK])
#define get_queen_mask(_side) (_side == WHITE ? board->bits[WHITE_QUEEN] : board->bits[BLACK_QUEEN])
#define get_king_mask(_side) (_side == WHITE ? board->bits[WHITE_KING] : board->bits[BLACK_KING])
#define get_piece(sq) (board->piece_at[sq] + (board->color_at[sq] == BLACK ? 6 : 0))
#define get_color(sq) (board->color_at[sq])
#define in_check() bool(board->king_attackers)

static inline bool move_is_straight(const Move move) {
	return row(get_from(move)) == row(get_to(move)) || col(get_from(move)) == col(get_to(move));
}

static inline bool move_is_diagonal(const Move move) {
	return abs(row(get_from(move)) - row(get_to(move))) == abs(col(get_from(move)) - col(get_to(move)));
}

Move MovePicker::get_random_move(const Board& board) {
	std::vector<Move> moves; 

	generate_moves(moves, &board);

    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng = std::default_random_engine { _seed };
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

	if(moves.empty()) {
		return NULL_MOVE;
	}

	int index = dist(rng) % (long long)moves.size();
	int iterations = 0;

	while(!board.fast_move_valid(moves[index])) {
		iterations++;
		index = dist(rng) % (long long)moves.size();
		assert(iterations < 1000);
	}

	assert(board.fast_move_valid(moves[index]));

	return moves[index];
}

uint64_t MovePicker::get_attackers(const int to_sq, const bool attacker_side) const { 
    uint64_t attackers = 0;

    if(attacker_side == BLACK) {
		if((mask_sq(to_sq) & ~COL_0) && (to_sq + 7) >= 0 && (to_sq + 7) < 64 && board->piece_at[to_sq + 7] == PAWN && board->color_at[to_sq + 7] == BLACK)
        	return to_sq + 7;
        if((mask_sq(to_sq) & ~COL_7) && (to_sq + 9) >= 0 && (to_sq + 9) < 64 && board->piece_at[to_sq + 9] == PAWN && board->color_at[to_sq + 9] == BLACK)
        	return to_sq + 9; 
    } else {
        if((mask_sq(to_sq) & ~COL_0) && (to_sq - 9) >= 0 && (to_sq - 9) < 64 && board->piece_at[to_sq - 9] == PAWN && board->color_at[to_sq - 9] == WHITE) 
        	return to_sq - 9; 
        if((mask_sq(to_sq) & ~COL_7) && (to_sq - 7) >= 0 && (to_sq - 7) < 64 && board->piece_at[to_sq - 7] == PAWN && board->color_at[to_sq - 7] == WHITE)
        	return to_sq - 7;
    }

    attackers |= Rmagic(to_sq, get_all_mask()) & (get_rook_mask(attacker_side) | get_queen_mask(attacker_side));
    attackers |= Bmagic(to_sq, get_all_mask()) & (get_bishop_mask(attacker_side) | get_queen_mask(attacker_side));
    attackers |= knight_attacks[to_sq] & get_knight_mask(attacker_side);

    return attackers;
}

// we assume attacker is side
// least valuable attacker - we assume that attacker side is 'side'
int MovePicker::lva(int to_sq) const {
    if(board->side == BLACK) {
		if((mask_sq(to_sq) & ~COL_0) && (to_sq + 7) >= 0 && (to_sq + 7) < 64 && get_piece(to_sq + 7) == BLACK_PAWN)
        	return to_sq + 7;
        if((mask_sq(to_sq) & ~COL_7) && (to_sq + 9) >= 0 && (to_sq + 9) < 64 && get_piece(to_sq + 9) == BLACK_PAWN)
        	return to_sq + 9; 
    } else {
        if((mask_sq(to_sq) & ~COL_0) && (to_sq - 9) >= 0 && (to_sq - 9) < 64 && get_piece(to_sq - 9) == WHITE_PAWN)
        	return to_sq - 9; 
        if((mask_sq(to_sq) & ~COL_7) && (to_sq - 7) >= 0 && (to_sq - 7) < 64 && get_piece(to_sq - 7) == WHITE_PAWN) 
        	return to_sq - 7;
    }
	if(knight_attacks[to_sq] & get_knight_mask(board->side))
		return lsb(knight_attacks[to_sq] & get_knight_mask(board->side));
    if(Rmagic(to_sq, get_all_mask()) & (get_rook_mask(board->side) | get_queen_mask(board->side)))
    	return lsb(Rmagic(to_sq, get_all_mask()) & (get_rook_mask(board->side) | get_queen_mask(board->side)));
    if(Bmagic(to_sq, get_all_mask()) & (get_bishop_mask(board->side) | get_queen_mask(board->side)))
    	return lsb(Bmagic(to_sq, get_all_mask()) & (get_bishop_mask(board->side) | get_queen_mask(board->side)));
    return -1;
}

int MovePicker::next_lva(const uint64_t& attacker_mask, const bool attacker_side) const {
	for(int piece = (attacker_side == WHITE ? WHITE_PAWN : BLACK_PAWN); piece < (attacker_side == WHITE ? BLACK_PAWN : 12); piece++) {
		if(attacker_mask & get_piece_mask(piece)) {
			return lsb(attacker_mask & get_piece_mask(piece));
		}
	} 
	return -1;
}

int MovePicker::fast_see(const Move move) const {
	int from_sq, to_sq, piece_at_to, depth, side;
	uint64_t attacker_mask, occ_mask, _diagonal_mask;
	std::vector<int> score(16, 0);

	to_sq = get_to(move);
	from_sq = get_from(move);
	depth = 0;
	side = board->side;
	piece_at_to = get_piece(to_sq);
	attacker_mask = get_attackers(to_sq, board->side) | get_attackers(to_sq, board->xside);	

	while(from_sq != -1) {
		score[depth + 1] = piece_value[piece_at_to] - score[depth];
		depth++;
		piece_at_to = get_piece(from_sq);
		attacker_mask ^= mask_sq(from_sq);
		side = !side;

		// check xrays
		if(move_is_diagonal(move)) {
			if(from_sq > to_sq) { // north
				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][NORTHEAST];
				else _diagonal_mask = diagonal_mask[from_sq][NORTHWEST];
			} else { // south
				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][SOUTHEAST];
				else _diagonal_mask = diagonal_mask[from_sq][SOUTHWEST];
			}

			attacker_mask |= Bmagic(from_sq, board->occ_mask) & _diagonal_mask
			& (get_queen_mask(side) | get_bishop_mask(side) | get_queen_mask(!side) | get_bishop_mask(!side)); 
		} else if(move_is_straight(move)) {
			if(row(from_sq) == row(to_sq)) {
				attacker_mask |= Rmagic(from_sq, board->occ_mask) & straight_mask[from_sq][from_sq < to_sq ? EAST : WEST]
				& (get_queen_mask(side) | get_rook_mask(side) | get_queen_mask(!side) | get_rook_mask(!side));
			} else {
				attacker_mask |= Rmagic(from_sq, board->occ_mask) & straight_mask[from_sq][from_sq < to_sq ? SOUTH : NORTH]
				& (get_queen_mask(side) | get_rook_mask(side) | get_queen_mask(!side) | get_rook_mask(!side));
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

int MovePicker::slow_see(const Move move, const bool root) {
	const int piece_from = get_piece(get_from(move));
	const int piece_captured = get_piece(get_to(move));
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

void MovePicker::delete_move(int index) {
	assert(!move_stack.empty());
	assert(scores.size() == move_stack.size());
	move_stack[index] = move_stack[move_stack.size() - 1];
	scores[index] = move_stack[scores.size() - 1];
	move_stack.pop_back();
	scores.pop_back();
}

int MovePicker::get_best_index(bool no_min) const {
	if(move_stack.empty()) {
		return -1;
	}

	int best_index = (no_min ? 0 : -1), best_score = -1;

	assert(scores.size() == move_stack.size());

	for(int index = 0; index < (int)scores.size(); index++) {
		if(scores[index] > best_score) {
			best_index = index;	
			best_score = scores[index];
		}
	}

	return best_index;
}

void MovePicker::sort_captures() {
	for(int i = 0; i < move_stack.size(); i++) {
		scores.push_back(
            thread->capture_history[board->piece_at[get_from(move_stack[i])]][get_to(move_stack[i])][board->piece_at[get_to(move_stack[i])]]
			+ 50000 + 50000 * (get_flag(move_stack[i]) == QUEEN_PROMOTION)
		);
		assert(scores[scores.size() - 1] > 0);
	}
	assert(move_stack.size() == scores.size());
} 

void MovePicker::sort_quiet() {
	for(int i = 0; i < move_stack.size(); i++) {
		scores.push_back(
            100000 + thread->quiet_history[board->side][get_from(move_stack[i])][get_to(move_stack[i])]
		);
	}	
	assert(move_stack.size() == scores.size());
}

Move MovePicker::next_move() {
	tt_move = NULL_MOVE;
	switch(phase) {
		case HASH: {
			phase = GENERATE_CAPTURES;
            tt_move = NULL_MOVE;
			if(tt_move != NULL_MOVE
			&& board->move_valid(tt_move)) {
				return tt_move;
			}
		}

		case GENERATE_CAPTURES: {
			// if we are in check we skip ahead to the evasions
			if(board->king_attackers) {
				assert(move_stack.empty());
				generate_evasions(move_stack, board);
				sort_quiet();
				phase = QUIET;
				return next_move();
			} else {
				assert(move_stack.empty());
				generate_captures(move_stack, board);
				sort_captures();
				phase = GOOD_CAPTURES;
			}
		}

		case GOOD_CAPTURES: {
			int best_index = get_best_index();		
			while(best_index != -1) {
				if(move_stack[best_index] == tt_move) {
				// || !board->fast_move_valid(move_stack[best_index])) {
					delete_move(best_index);	
					best_index = get_best_index();
				} else if(get_flag(move_stack[best_index]) != ENPASSANT_MOVE
				&& board->piece_at[get_from(move_stack[best_index])] < board->piece_at[get_from(move_stack[best_index])] 
				&& fast_see(move_stack[best_index]) < 0) { 
				// } else if(fast_see(move_stack[best_index]) <= 0) {
					scores[best_index] = -1;
					best_index = get_best_index();
				} else {
					const Move move = move_stack[best_index];
					delete_move(best_index);
					return move;
				}
			}
			assert(scores.empty() || scores[0] == -1);
			phase = FIRST_KILLER;
		}

		case FIRST_KILLER: {
			phase = SECOND_KILLER;
            if(!captures_only) {
				if(thread->killers[thread->ply][0] != NULL_MOVE
				&& thread->killers[thread->ply][0] != tt_move
				&& board->move_valid(thread->killers[thread->ply][0])) {
					return thread->killers[thread->ply][0];
				}
            }
		}

		case SECOND_KILLER: {
			phase = BAD_CAPTURES;
			if(!captures_only) {
				if(thread->killers[thread->ply][1] != NULL_MOVE
				&& thread->killers[thread->ply][1] != tt_move
				&& thread->killers[thread->ply][1] != thread->killers[thread->ply][0] 
				&& board->move_valid(thread->killers[thread->ply][1])) {
					return thread->killers[thread->ply][1];
				}
			}
		}

		case BAD_CAPTURES: {
			while(!move_stack.empty()) {
				const Move move = move_stack[0];
				assert(thread->board.fast_move_valid(move));
				delete_move(0);
				if(move != tt_move) {
					return move;
				}
			}	
			assert(move_stack.empty());
			if(captures_only) {
				phase = DONE;
				return NULL_MOVE;
			}
			phase = GENERATE_QUIET;
		}

		case GENERATE_QUIET: {
			assert(move_stack.empty());
			generate_quiet(move_stack, board);
			sort_quiet();
			phase = QUIET;
		}

		case QUIET: {
			int best_index = get_best_index(true);
			while(best_index != -1) {
				const Move move = move_stack[best_index];
				delete_move(best_index);
				assert(bool(thread->board.king_attackers) || get_flag(move) != CAPTURE_MOVE);
				if(move != tt_move) {
					return move;
				}
				best_index = get_best_index(true);
			}
			assert(move_stack.empty());
		}
	}

	assert(move_stack.empty());

	return NULL_MOVE;
}
