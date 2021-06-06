#include <sys/timeb.h>
#include "defs.h"
#include "bitboard.h"
#include "magicmoves.h"
#include "sungorus_eval.h"
#include "board.h"
#include "gen.h"

static bool mat_ok(Board* board) {
	return calculate_mat_p(board) == (board->b_mat[0] - board->b_mat[1] + board->b_pst[0] - board->b_pst[1]);
}

static const int pst[6][64] = {
  { 0, 4, 8, 10, 10, 8, 4, 0, 4, 8, 12, 14, 14, 12, 8, 4, 8, 12, 16, 18, 18, 16, 12, 8, 10, 14, 18, 20, 20, 18, 14, 10, 10, 14, 18, 20, 20, 18, 14, 10, 8, 12, 16, 18, 18, 16, 12, 8, 4, 8, 12, 14, 14, 12, 8, 4, 0, 4, 8, 10, 10, 8, 4, 0 },
  { 0, 8, 16, 20, 20, 16, 8, 0, 8, 16, 24, 28, 28, 24, 16, 8, 16, 24, 32, 36, 36, 32, 24, 16, 20, 28, 36, 40, 40, 36, 28, 20, 20, 28, 36, 40, 40, 36, 28, 20, 16, 24, 32, 36, 36, 32, 24, 16, 8, 16, 24, 28, 28, 24, 16, 8, 0, 8, 16, 20, 20, 16, 8, 0 },
  { 0, 4, 8, 10, 10, 8, 4, 0, 4, 8, 12, 14, 14, 12, 8, 4, 8, 12, 16, 18, 18, 16, 12, 8, 10, 14, 18, 20, 20, 18, 14, 10, 10, 14, 18, 20, 20, 18, 14, 10, 8, 12, 16, 18, 18, 16, 12, 8, 4, 8, 12, 14, 14, 12, 8, 4, 0, 4, 8, 10, 10, 8, 4, 0 },
  { 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0, 0, 2, 4, 5, 5, 4, 2, 0 },
  { 0, 2, 4, 5, 5, 4, 2, 0, 2, 4, 6, 7, 7, 6, 4, 2, 4, 6, 8, 9, 9, 8, 6, 4, 5, 7, 9, 10, 10, 9, 7, 5, 5, 7, 9, 10, 10, 9, 7, 5, 4, 6, 8, 9, 9, 8, 6, 4, 2, 4, 6, 7, 7, 6, 4, 2, 0, 2, 4, 5, 5, 4, 2, 0 },
  { 0, 12, 24, 30, 30, 24, 12, 0, 12, 24, 36, 42, 42, 36, 24, 12, 24, 36, 48, 54, 54, 48, 36, 24, 30, 42, 54, 60, 60, 54, 42, 30, 30, 42, 54, 60, 60, 54, 42, 30, 24, 36, 48, 54, 54, 48, 36, 24, 12, 24, 36, 42, 42, 36, 24, 12, 0, 12, 24, 30, 30, 24, 12, 0 }
};

#define get_side_mask(_side) (_side == WHITE ? \
	(bits[WHITE_PAWN] | bits[WHITE_KNIGHT] | bits[WHITE_BISHOP] | bits[WHITE_ROOK] | bits[WHITE_QUEEN] | bits[WHITE_KING]) : \
	(bits[BLACK_PAWN] | bits[BLACK_KNIGHT] | bits[BLACK_BISHOP] | bits[BLACK_ROOK] | bits[BLACK_QUEEN] | bits[BLACK_KING]))
#define get_piece_mask(piece) bits[piece]
#define get_all_mask() occ_mask
#define get_pawn_mask(_side) (_side == WHITE ? bits[WHITE_PAWN] : bits[BLACK_PAWN])
#define get_knight_mask(_side) (_side == WHITE ? bits[WHITE_KNIGHT] : bits[BLACK_KNIGHT])
#define get_bishop_mask(_side) (_side == WHITE ? bits[WHITE_BISHOP] : bits[BLACK_BISHOP])
#define get_rook_mask(_side) (_side == WHITE ? bits[WHITE_ROOK] : bits[BLACK_ROOK])
#define get_queen_mask(_side) (_side == WHITE ? bits[WHITE_QUEEN] : bits[BLACK_QUEEN])
#define get_king_mask(_side) (_side == WHITE ? bits[WHITE_KING] : bits[BLACK_KING])
#define get_piece(sq) (piece_at[sq] + (color_at[sq] ? 6 : 0))
#define get_color(sq) (color_at[sq])
#define in_check() bool(king_attackers)

void Board::new_take_back(const UndoData& undo) {
	uint8_t from_sq, to_sq, piece, captured_piece;

	from_sq = get_from(undo.move);
	to_sq = get_to(undo.move);
	piece = undo.moved_piece - (xside ? 6 : 0);
	captured_piece = undo.captured_piece == EMPTY ? EMPTY : undo.captured_piece - (side ? 6 : 0);

	assert((bits[BLACK_PAWN] & ROW_0) == 0);
	assert(get_flag(undo.move) == CASTLING_MOVE || captured_piece == EMPTY || (captured_piece >= WHITE_PAWN && captured_piece < BLACK_PAWN));

    enpassant = undo.enpassant;

	castling_flag = undo.castling_flag;

    switch(get_flag(undo.move)) {
        case NULL_MOVE:
            break;
        case QUIET_MOVE: {
			b_pst[xside] += pst[piece][from_sq] - pst[piece][to_sq];
			color_at[from_sq] = xside;
			piece_at[from_sq] = piece;
			color_at[to_sq] = piece_at[to_sq] = EMPTY;
			bits[undo.moved_piece] ^= mask_sq(to_sq) | mask_sq(from_sq);
			occ_mask ^= mask_sq(to_sq) | mask_sq(from_sq);
            break;
		}
        case CAPTURE_MOVE: {
			b_pst[xside] += pst[piece][from_sq] - pst[piece][to_sq];
			b_pst[side] += pst[captured_piece][to_sq];
			b_mat[side] += piece_value[captured_piece];
			color_at[from_sq] = xside;
			piece_at[from_sq] = piece;
			color_at[to_sq] = side;
			piece_at[to_sq] = captured_piece;
			assert(piece_at[to_sq] >= 0);
			bits[undo.moved_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			bits[undo.captured_piece] ^= mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq);
            break;
		}
        case CASTLING_MOVE: {
			uint8_t rook_from, rook_to;
			if(from_sq > to_sq) {
				rook_to = to_sq + 1;
				rook_from = from_sq - 4;
			} else {
				rook_to = to_sq - 1;
				rook_from = from_sq + 3;
			}
			b_pst[xside] += pst[KING][from_sq] - pst[KING][to_sq]
						  + pst[ROOK][rook_from] - pst[ROOK][rook_to];
			color_at[from_sq] = color_at[rook_from] = xside;
			piece_at[from_sq] = KING;
			piece_at[rook_from] = ROOK;
			color_at[to_sq] = piece_at[to_sq] = color_at[rook_to] = piece_at[rook_to] = EMPTY;
			bits[undo.moved_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			bits[ROOK + (xside ? 6 : 0)] ^= mask_sq(rook_from) | mask_sq(rook_to);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq) | mask_sq(rook_from) | mask_sq(rook_to);
            break;
		}
        case ENPASSANT_MOVE: {
			const uint8_t adjacent = (row(from_sq) << 3) | col(to_sq);
			assert(piece == PAWN);
			b_pst[xside] += pst[PAWN][from_sq] - pst[PAWN][to_sq];
			b_pst[side] += pst[PAWN][adjacent];
			b_mat[side] += piece_value[PAWN];
			color_at[from_sq] = xside;
			piece_at[from_sq] = PAWN;
			color_at[adjacent] = side;
			piece_at[adjacent] = PAWN;
			color_at[to_sq] = piece_at[to_sq] = EMPTY;
			bits[undo.moved_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			bits[undo.captured_piece] ^= mask_sq(adjacent);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq) | mask_sq(adjacent);
            break;
		}		
		// default: {
        case QUEEN_PROMOTION: case ROOK_PROMOTION: case KNIGHT_PROMOTION: case BISHOP_PROMOTION: {
			const int8_t promotion_piece = get_flag(undo.move) - KNIGHT_PROMOTION + KNIGHT; 
			piece_at[to_sq] = color_at[to_sq] = EMPTY;
			if(undo.captured_piece != EMPTY) {
				b_pst[side] += pst[captured_piece][to_sq];
				b_mat[side] += piece_value[captured_piece];
				color_at[to_sq] = side;
				piece_at[to_sq] = captured_piece;
				assert(piece_at[to_sq] >= 0);
				assert(undo.captured_piece != 6);
				bits[undo.captured_piece] ^= mask_sq(to_sq);
				occ_mask ^= mask_sq(to_sq);
			}
			b_pst[xside] += pst[PAWN][from_sq] - pst[promotion_piece][to_sq];
			b_mat[xside] += piece_value[PAWN] - piece_value[promotion_piece];
			color_at[from_sq] = xside;
			piece_at[from_sq] = PAWN;
			bits[undo.moved_piece] ^= mask_sq(from_sq);
			assert(undo.moved_piece == 0 || undo.moved_piece == 6);
			bits[promotion_piece + (xside ? 6 : 0)] ^= mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq);
			assert(!(bits[undo.moved_piece] & (ROW_0 | ROW_7)));
		}
    }

	fifty_move_ply = undo.fifty_move_ply;

	if(side)
		move_count--;

	assert(move_count >= 0);

    assert(!keys.empty());
    keys.pop_back();
    key = keys.back();

    side = xside;
	xside = !xside;

	// we store this in undo data now
	// king_attackers = undo.king_attackers;
	king_attackers = undo.king_attackers;
}

void Board::new_make_move(const Move move, UndoData& undo_data) {
	uint8_t from_sq, to_sq, piece, side_piece, side_shift;

	from_sq = get_from(move);
	to_sq = get_to(move);
	piece = piece_at[from_sq];
	side_piece = piece_at[from_sq] + (color_at[from_sq] ? 6 : 0);
	side_shift = (side ? 6 : 0);
	assert(side_piece == get_piece(from_sq));

	undo_data.move = move;
	undo_data.enpassant = enpassant;
	undo_data.castling_flag = castling_flag;
	undo_data.moved_piece = side_piece;
	undo_data.captured_piece = EMPTY;
	undo_data.fifty_move_ply = fifty_move_ply;

	if(enpassant != NO_ENPASSANT) {
		key ^= zobrist_enpassant[enpassant]; 
		enpassant = NO_ENPASSANT;
	}

	key ^= zobrist_castling[castling_flag];
	castling_flag &= castling_bitmasks[from_sq] & castling_bitmasks[to_sq];
	key ^= zobrist_castling[castling_flag];

	switch(get_flag(move)) {
		case NULL_MOVE:
			break;
		case QUIET_MOVE: {
			assert(piece_at[to_sq] == EMPTY);
			b_pst[side] += pst[piece][to_sq] - pst[piece][from_sq];
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[side_piece][to_sq];
			color_at[to_sq] = side;
			piece_at[to_sq] = piece;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq);
			if(!piece && abs(from_sq - to_sq) == 16) {
				enpassant = col(from_sq);
				key ^= zobrist_enpassant[enpassant]; 
			}
			break;
		}
		case CAPTURE_MOVE: {
			undo_data.captured_piece = get_piece(to_sq);
			b_pst[side] += pst[piece][to_sq] - pst[piece][from_sq];
			b_pst[xside] -= pst[piece_at[to_sq]][to_sq];
			b_mat[xside] -= piece_value[piece_at[to_sq]];
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[side_piece][to_sq] ^ zobrist_pieces[undo_data.captured_piece][to_sq];
			color_at[to_sq] = side;
			piece_at[to_sq] = piece;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			bits[undo_data.captured_piece] ^= mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq);
			break;
		}
		case CASTLING_MOVE: { 
			undo_data.captured_piece = ROOK + side_shift; // I don't remember why we do this
			b_pst[side] += pst[KING][to_sq] - pst[KING][from_sq];
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[side_piece][to_sq]; 
			color_at[to_sq] = side;
			piece_at[to_sq] = KING;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq);
			// we do the same but with the rook
			if(from_sq > to_sq) {
				to_sq += 1;
				from_sq -= 4;
			} else {
				to_sq -= 1; 
				from_sq += 3;
			}
			b_pst[side] += pst[ROOK][to_sq] - pst[ROOK][from_sq];
			key ^= zobrist_pieces[ROOK + side_shift][from_sq] ^ zobrist_pieces[ROOK + side_shift][to_sq]; 
			color_at[to_sq] = side;
			piece_at[to_sq] = ROOK;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[ROOK + side_shift] ^= mask_sq(from_sq) | mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq);
			break;
		}
		case ENPASSANT_MOVE: {
			const int adjacent = (row(from_sq) << 3) | col(to_sq);
			undo_data.captured_piece = PAWN + (side ? 0 : 6);
			b_pst[side] += pst[piece][to_sq] - pst[piece][from_sq];
			b_pst[xside] -= pst[piece_at[adjacent]][adjacent];
			b_mat[xside] -= piece_value[PAWN];
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[side_piece][to_sq] ^ zobrist_pieces[undo_data.captured_piece][adjacent];
			color_at[to_sq] = side;
			piece_at[to_sq] = PAWN;
			color_at[from_sq] = piece_at[from_sq] = color_at[adjacent] = piece_at[adjacent] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			bits[undo_data.captured_piece] ^= mask_sq(adjacent);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq) | mask_sq(adjacent);
			break;
		}
		// default: { 
		case KNIGHT_PROMOTION: case BISHOP_PROMOTION: case ROOK_PROMOTION: case QUEEN_PROMOTION: { 
			const int promotion_piece = KNIGHT + get_flag(move) - KNIGHT_PROMOTION; 
			if(piece_at[to_sq] != EMPTY) { // promotion with capture
				undo_data.captured_piece = get_piece(to_sq);
				b_pst[xside] -= pst[piece_at[to_sq]][to_sq];	
				b_mat[xside] -= piece_value[piece_at[to_sq]];
				key ^= zobrist_pieces[undo_data.captured_piece][to_sq]; 
				bits[undo_data.captured_piece] ^= mask_sq(to_sq);
				occ_mask ^= mask_sq(to_sq);
			}
			b_pst[side] += pst[promotion_piece][to_sq] - pst[PAWN][from_sq];
			b_mat[side] += piece_value[promotion_piece] - piece_value[PAWN];
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[promotion_piece + side_shift][to_sq]; 
			color_at[to_sq] = side;
			piece_at[to_sq] = promotion_piece;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq); 
			bits[promotion_piece + side_shift] ^= mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq);
		}
	}	

	fifty_move_ply++;

	if(piece == PAWN || get_flag(move) == CAPTURE_MOVE)
		fifty_move_ply = 0;

	if(side)
		move_count++;

	xside = side;
	side = !side;
	key ^= zobrist_side[side] ^ zobrist_side[xside];

    keys.push_back(key);

    king_attackers = get_attackers(lsb(get_king_mask(side)), xside, this);
}

bool Board::new_fast_move_valid(const Move move) const {
	uint8_t piece_from, piece_to, from_sq, to_sq, flag, side_shift, xside_shift, king_sq;

	piece_from = get_piece(get_from(move));
	piece_to = get_piece(get_to(move));
	from_sq = get_from(move);
	to_sq = get_to(move);
	flag = get_flag(move);
	side_shift = side ? 6 : 0;
	xside_shift = xside ? 6 : 0;

	// This code underneath is equivalent to just doing:
	// make_move(move);
	// bool in_check_after_move = xside_king_attacked();
	// take_back(move);
	// return !in_check_after_move;
	// the problem is that we want the function to be const so we can't touch the board
   
	if(piece_from == WHITE_KING || piece_from == BLACK_KING) {
		assert(!(get_flag(move) == CASTLING_MOVE && king_attackers));
		return !(knight_attacks[to_sq] & bits[KNIGHT + xside_shift])
			&& !(king_attacks[to_sq] & bits[KING + xside_shift])
			&& !(pawn_attacks[xside][to_sq] & bits[PAWN + xside_shift])
			&& !(Bmagic(to_sq, occ_mask ^ mask_sq(from_sq)) & (bits[BISHOP + xside_shift] | bits[QUEEN + xside_shift]))
			&& !(Rmagic(to_sq, occ_mask ^ mask_sq(from_sq)) & (bits[ROOK + xside_shift] | bits[QUEEN + xside_shift]));
	}
		
	// if we are here then the moved piece isn't the king
	king_sq = lsb(bits[KING + side_shift]);

	if(get_flag(move) == ENPASSANT_MOVE) {
		const uint8_t adjacent = (row(from_sq) << 3) | col(to_sq); 

		// if a pawn is attacking the king and you don't eat it with the enpassant move it's an invalid move
		return !(knight_attacks[king_sq] & (bits[KNIGHT + xside_shift] & ~mask_sq(to_sq))) 
			&& !(pawn_attacks[xside][king_sq] & bits[PAWN + xside_shift] & ~mask_sq(adjacent)) 
			&& !(Bmagic(king_sq, occ_mask ^ mask_sq(from_sq) ^ mask_sq(adjacent) ^ mask_sq(to_sq)) & (bits[BISHOP + xside_shift] | bits[QUEEN + xside_shift]))
			&& !(Rmagic(king_sq, occ_mask ^ mask_sq(from_sq) ^ mask_sq(adjacent) ^ mask_sq(to_sq)) & (bits[ROOK   + xside_shift] | bits[QUEEN + xside_shift]));
	}

	return !(knight_attacks[king_sq] & (bits[KNIGHT + xside_shift] & ~mask_sq(to_sq))) 
		&& !(pawn_attacks[xside][king_sq] & (bits[PAWN + xside_shift] & ~mask_sq(to_sq))) 
		&& !(Bmagic(king_sq, occ_mask ^ mask_sq(from_sq) | mask_sq(to_sq)) & (~mask_sq(to_sq) & (bits[BISHOP + xside_shift] | bits[QUEEN + xside_shift])))
		&& !(Rmagic(king_sq, occ_mask ^ mask_sq(from_sq) | mask_sq(to_sq)) & (~mask_sq(to_sq) & (bits[ROOK   + xside_shift] | bits[QUEEN + xside_shift])));
}


bool Board::new_move_valid(const Move move) {
	uint8_t from_sq, to_sq, flag, piece, side_shift;
	bool is_promotion;

	from_sq = get_from(move);
	to_sq = get_to(move);
	flag = get_flag(move);
	is_promotion = KNIGHT_PROMOTION <= flag && flag <= QUEEN_PROMOTION;
	side_shift = (side ? 6 : 0);

	if(from_sq < 0 || from_sq >= 64
	|| to_sq < 0 || to_sq >= 64
	|| (flag == QUIET_MOVE && color_at[to_sq] != EMPTY)
	|| (flag == CAPTURE_MOVE && color_at[to_sq] != xside)
	|| (flag < NULL_MOVE || flag > QUEEN_PROMOTION)) {
		return false;
	}

	piece = get_piece(from_sq);

	// trivial conditions that must be met
	if(from_sq == to_sq || piece == EMPTY || color_at[from_sq] != side || color_at[to_sq] == side)
		return false;

	if(flag == CASTLING_MOVE) {
		if(king_attackers)
			return false;

		// this is the only place were we return true before the end of the function
		uint8_t castling_type;

		if(side == WHITE
		&& (from_sq == E1 && to_sq == C1)
		&& (castling_flag & 1)
		&& !(occ_mask & castling_mask[WHITE_QUEEN_SIDE])) { 
			castling_type = WHITE_QUEEN_SIDE;
		} else if(side == WHITE
		&& (from_sq == E1 && to_sq == G1)
		&& (castling_flag & 2)
		&& !(occ_mask & castling_mask[WHITE_KING_SIDE])) {
			castling_type = WHITE_KING_SIDE;
		} else if(side == BLACK
		&& (from_sq == E8 && to_sq == C8)
		&& (castling_flag & 4)
		&& !(occ_mask & castling_mask[BLACK_QUEEN_SIDE])) {
			castling_type = BLACK_QUEEN_SIDE;
		} else if(side == BLACK
		&& (from_sq == E8 && to_sq == G8)
		&& (castling_flag & 8)	
		&& !(occ_mask & castling_mask[BLACK_KING_SIDE])) { 
			castling_type = BLACK_KING_SIDE;
		} else {
			return false;
		}

		// we don't check whether the position to_sq is attacked
		// we will check that later using fast_move_valid

		if(castling_type == WHITE_QUEEN_SIDE || castling_type == BLACK_QUEEN_SIDE) { 
			return !is_attacked(from_sq - 1);
		} else if(castling_type == WHITE_KING_SIDE || castling_type == BLACK_KING_SIDE) {
			return !is_attacked(from_sq + 1);
		} else {
			return false;
		}
		
	} else if(piece_at[from_sq] == PAWN) {
		// check for diagonal movement
		if(flag == CAPTURE_MOVE || flag == ENPASSANT_MOVE || (is_promotion && color_at[to_sq] != EMPTY)) {
			if(side == WHITE) { 
				if(get_to(move) <= get_from(move) 
				|| abs(row(get_from(move)) - row(get_to(move))) != 1
				|| abs(col(get_from(move)) - col(get_to(move))) != 1)
					return false;
			} else {
				if(get_to(move) >= get_from(move) 
				|| abs(row(get_from(move)) - row(get_to(move))) != 1
				|| abs(col(get_from(move)) - col(get_to(move))) != 1)
					return false;
			}
		} else if(flag != QUIET_MOVE && !is_promotion) {
			return false; // invalid flag
		} 
		
		if(flag == QUIET_MOVE) {
			if(!((side == WHITE && to_sq == from_sq + 8)
			|| (side == WHITE && row(from_sq) == 1 && to_sq == from_sq + 16)
			|| (side == BLACK && to_sq == from_sq - 8)
			|| (side == BLACK && row(from_sq) == 6 && to_sq == from_sq - 16))) {
				return false;
			}
			if(to_sq == from_sq + 16 && piece_at[from_sq + 8] != EMPTY) {
				return false;
			}
			if(to_sq == from_sq - 16 && piece_at[from_sq - 8] != EMPTY) {
				return false;
			}
		} else if(flag == ENPASSANT_MOVE) {
			const uint8_t adjacent = (row(from_sq) << 3) | col(to_sq);
			if(enpassant != col(adjacent)
			|| (side == WHITE && row(to_sq) != 5)
			|| (side == BLACK && row(to_sq) != 2))
				return false;
			// if this fails there is a bug regarding the enpassant flag	
			assert(piece_at[adjacent] == PAWN && color_at[adjacent] == xside);
		} else if(is_promotion) {
			// must be at the last/first row
			if((side == WHITE && row(from_sq) == 6 && row(to_sq) != 7) || (side == BLACK && row(from_sq) == 1 && row(to_sq) != 0))
				return false;
			if(piece_at[to_sq] == EMPTY
			&& ((side == WHITE && to_sq != from_sq + 8) || (side == BLACK && to_sq != from_sq - 8)))
				return false;
		}
	} else {
		switch(piece - side_shift) {
			case KING: { if(!(mask_sq(to_sq) & king_attacks[from_sq])) return false; break; }
			case KNIGHT: { if(!(mask_sq(to_sq) & knight_attacks[from_sq])) return false; break; } 
			case BISHOP: { if(!(mask_sq(to_sq) & Bmagic(from_sq, occ_mask))) return false; break; }
			case ROOK: { if(!(mask_sq(to_sq) & Rmagic(from_sq, occ_mask))) return false; break; }
			case QUEEN: { if(!(mask_sq(to_sq) & (Bmagic(from_sq, occ_mask) | Rmagic(from_sq, occ_mask)))) return false; break; }
		}
	}


	// update: we no longer simulate the make move function now
	// we will check whether we put our king in check using fast_move_valid

	return true;
}


///////////////////////////////
//							 //
//   For testing the SEE     //
//   function and its        //
//   subfunctions			 //
//							 //
///////////////////////////////

static inline bool move_is_straight(int from_sq, int to_sq) {
	return row(from_sq) == row(to_sq) || col(from_sq) == col(to_sq);
}

static inline bool move_is_diagonal(int from_sq, int to_sq) {
	return abs(row(from_sq) - row(to_sq)) == abs(col(from_sq) - col(to_sq));
}

// duplicated from move_picker
int Board::next_lva(const uint64_t& attacker_mask, const bool attacker_side) const {
	for(uint8_t piece = (attacker_side == WHITE ? WHITE_PAWN : BLACK_PAWN); piece < (attacker_side == WHITE ? BLACK_PAWN : 12); piece++) {
		if(attacker_mask & bits[piece]) {
			return lsb(attacker_mask & bits[piece]);
		}
	} 
	return -1;
}

// we assume attacker is side
// least valuable attacker - we assume that attacker side is 'side'
int Board::lva(int to_sq) const {
    if(side == BLACK) {
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
	const uint8_t side_shift = side ? 6 : 0;
	if(knight_attacks[to_sq] & bits[KNIGHT + side_shift])
		return lsb(knight_attacks[to_sq] & bits[KNIGHT + side_shift]);
    if(Rmagic(to_sq, occ_mask) & (bits[ROOK + side_shift] | bits[QUEEN + side_shift]))
    	return lsb(Rmagic(to_sq, occ_mask) & (bits[ROOK + side_shift] | bits[QUEEN + side_shift]));
    if(Bmagic(to_sq, occ_mask) & (bits[BISHOP + side_shift] | bits[QUEEN + side_shift]))
    	return lsb(Bmagic(to_sq, occ_mask) & (bits[BISHOP + side_shift] | bits[QUEEN + side_shift]));
    return -1;
}

int Board::fast_see(const Move move) const {
	uint8_t from_sq, to_sq, piece_at_to, depth, _side, i;
	uint64_t attacker_mask = 0;
	int score[16] = { 0 };

	to_sq = get_to(move);
	from_sq = get_from(move);
	depth = 0;
	piece_at_to = get_piece(to_sq);
	get_attackers(to_sq, side, this, attacker_mask);
	get_attackers(to_sq, xside, this, attacker_mask);
	_side = side;

	while(from_sq != 64) {
		score[depth + 1] = piece_value[piece_at_to] - score[depth];
		depth++;
		piece_at_to = get_piece(from_sq);
		attacker_mask ^= mask_sq(from_sq);
		_side = !_side;

		// check xrays
		if(abs(row(from_sq) - row(to_sq)) == abs(col(from_sq) - col(to_sq))) {
			if(from_sq > to_sq) { // north
				attacker_mask |= Bmagic(from_sq, occ_mask) & diagonal_mask[from_sq][(col(from_sq) < col(to_sq)) ? NORTHEAST : NORTHWEST]
				& (bits[WHITE_BISHOP] | bits[BLACK_BISHOP] | bits[WHITE_QUEEN] | bits[BLACK_QUEEN]);
			} else { // south
				attacker_mask |= Bmagic(from_sq, occ_mask) & diagonal_mask[from_sq][(col(from_sq) < col(to_sq)) ? SOUTHEAST : SOUTHWEST]
				& (bits[WHITE_BISHOP] | bits[BLACK_BISHOP] | bits[WHITE_QUEEN] | bits[BLACK_QUEEN]);
			}
		} else if(row(from_sq) == row(to_sq)) {
			attacker_mask |= Rmagic(from_sq, occ_mask) & straight_mask[from_sq][from_sq < to_sq ? EAST : WEST]
			& (bits[WHITE_ROOK] | bits[BLACK_ROOK] | bits[WHITE_QUEEN] | bits[BLACK_QUEEN]);
		} else if(col(from_sq) == col(to_sq)) {
			attacker_mask |= Rmagic(from_sq, occ_mask) & straight_mask[from_sq][from_sq < to_sq ? SOUTH : NORTH]
			& (bits[WHITE_ROOK] | bits[BLACK_ROOK] | bits[WHITE_QUEEN] | bits[BLACK_QUEEN]);
		}
		
		from_sq = 64; // if we don't find any more attacker we stop the while loop
		for(i = WHITE_PAWN + (_side ? 6 : 0); i < BLACK_PAWN + (_side ? 6 : 0); i++) if(attacker_mask & bits[i]) {
			from_sq = lsb(attacker_mask & bits[i]);
			break;
		} 

		// the king has put himself in check
		if(from_sq != 64 && (piece_at_to == WHITE_KING || piece_at_to == BLACK_KING)) {
			depth--;
			from_sq = 64;
		}
	}

	score[0] = 10000; // we want to force the capture at the root
	score[depth + 1] = -score[depth];

	while(depth) {
		// (Kind of) minimax optimization. At each node you can choose between not making the capture
		// or making the capture and assuming that the other side will play optimally after that
		score[depth] = std::max(-score[depth - 1], -score[depth + 1]);
		depth--;
	}

	return score[1];
}
