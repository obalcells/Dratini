#include <sys/timeb.h>
#include "defs.h"
#include "bitboard.h"
#include "magicmoves.h"
#include "board.h"
#include "gen.h"

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
#define get_piece(sq) (piece_at[sq] + (color_at[sq] == BLACK ? 6 : 0))
#define get_color(sq) (color_at[sq])
#define in_check() bool(king_attackers)

void Board::new_take_back(const UndoData& undo) {
	uint8_t from_sq, to_sq, piece, captured_piece;

	from_sq = get_from(undo.move);
	to_sq = get_to(undo.move);
	piece = undo.moved_piece - (xside ? 6 : 0);
	captured_piece = undo.captured_piece == EMPTY ? EMPTY : undo.captured_piece - (side ? 6 : 0);

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
			int rook_from, rook_to;
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
		default: {
        // case QUEEN_PROMOTION: case ROOK_PROMOTION: case KNIGHT_PROMOTION: case BISHOP_PROMOTION: {
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
			assert((bits[undo.moved_piece] & ROW_0) == 0);
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

	// king_attackers = undo.king_attackers;
}

void Board::new_make_move(const Move move, UndoData& undo_data) {
	int from_sq, to_sq, piece, side_piece;

	from_sq = get_from(move);
	to_sq = get_to(move);
	piece = piece_at[from_sq];
	side_piece = get_piece(from_sq); // like piece but +6 if it's BLACK 

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
			undo_data.captured_piece = ROOK + (side ? 6 : 0); // I don't remember why we do this
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
			key ^= zobrist_pieces[ROOK + (side ? 6 : 0)][from_sq] ^ zobrist_pieces[ROOK + (side ? 6 : 0)][to_sq]; 
			color_at[to_sq] = side;
			piece_at[to_sq] = ROOK;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[ROOK + (side ? 6 : 0)] ^= mask_sq(from_sq) | mask_sq(to_sq);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq);
			break;
		}
		case ENPASSANT_MOVE: {
			const int adjacent = (row(from_sq) << 3) | col(to_sq);
			undo_data.captured_piece = PAWN + (side ? 0 : 6);
			b_pst[side] += pst[piece][to_sq] - pst[piece][from_sq];
			b_pst[xside] -= pst[piece_at[adjacent]][to_sq];
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
		default: { 
		// case KNIGHT_PROMOTION: case BISHOP_PROMOTION: case ROOK_PROMOTION: case QUEEN_PROMOTION:
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
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[promotion_piece + (side ? 6 : 0)][to_sq]; 
			color_at[to_sq] = side;
			piece_at[to_sq] = promotion_piece;
			color_at[from_sq] = piece_at[from_sq] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq); 
			bits[promotion_piece + (side ? 6 : 0)] ^= mask_sq(to_sq);
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

bool Board::fast_move_valid(const Move move) const {
	const int piece_from = get_piece(get_from(move));
	const int piece_to = get_piece(get_to(move));
	const int from_sq = get_from(move);
	const int to_sq = get_to(move);
	const int flag = get_flag(move);

	if(from_sq < 0 || from_sq > 63
	|| to_sq < 0 || to_sq > 63
	|| (flag == QUIET_MOVE && get_piece(to_sq) != EMPTY)
	|| (flag == CAPTURE_MOVE && get_piece(to_sq) == EMPTY)
	|| (get_color(from_sq) != side)
	|| (get_color(to_sq) == side)) {
		return false;
	}

	// This code underneath is equivalent to just doing:
	// make_move(move);
	// bool in_check_after_move = in_check();
	// take_back(move);
	// return !in_check_after_move;
	// the problem is that we want the function to be const so we can't touch the board
   
	if(piece_from == WHITE_KING || piece_from == BLACK_KING) {
		
		if((knight_attacks[to_sq] & get_knight_mask(xside))
		|| (king_attacks[to_sq] & get_king_mask(xside))
		|| (pawn_attacks[xside][to_sq] & get_pawn_mask(xside)))
			return false;

		const uint64_t occupation = (get_all_mask() ^ mask_sq(from_sq)) | mask_sq(to_sq);

		if(Bmagic(to_sq, occupation) & (get_bishop_mask(xside) | get_queen_mask(xside))
		|| Rmagic(to_sq, occupation) & (get_rook_mask(xside) | get_queen_mask(xside)))
			return false;

	} else {
		int king_sq = lsb(get_king_mask(side));

		if((knight_attacks[king_sq] & (get_knight_mask(xside) & ~mask_sq(to_sq)))
		|| (king_attacks[king_sq] & (get_king_mask(xside) & ~mask_sq(to_sq))))
			return false;

		if(get_flag(move) == ENPASSANT_MOVE) {
			if(pawn_attacks[xside][king_sq] & get_pawn_mask(xside) & ~mask_sq(side == WHITE ? (to_sq - 8) : (to_sq + 8)))
				return false;

			const uint64_t occupation = (get_all_mask() ^ mask_sq(from_sq) ^ mask_sq(side == WHITE ? (to_sq - 8) : (to_sq + 8))) | mask_sq(to_sq);

			if(Bmagic(king_sq, occupation) & (~mask_sq(to_sq) & (get_bishop_mask(xside) | get_queen_mask(xside)))
			|| Rmagic(king_sq, occupation) & (~mask_sq(to_sq) & (get_rook_mask(xside) | get_queen_mask(xside))))
				return false;

		} else {
			if(pawn_attacks[xside][king_sq] & (get_pawn_mask(xside) & ~mask_sq(to_sq)))
				return false;

			const uint64_t occupation = (get_all_mask() ^ mask_sq(from_sq)) | mask_sq(to_sq);

			if(Bmagic(king_sq, occupation) & (~mask_sq(to_sq) & (get_bishop_mask(xside) | get_queen_mask(xside)))
			|| Rmagic(king_sq, occupation) & (~mask_sq(to_sq) & (get_rook_mask(xside) | get_queen_mask(xside))))
				return false;
		}
	}

	return true;
}



///////////////////////////////
//
//   For testing the SEE
//   function and its
//   subfunctions
//
///////////////////////////////

// static inline bool move_is_straight(const Move move) {
// 	return row(get_from(move)) == row(get_to(move)) || col(get_from(move)) == col(get_to(move));
// }

// static inline bool move_is_diagonal(const Move move) {
// 	return abs(row(get_from(move)) - row(get_to(move))) == abs(col(get_from(move)) - col(get_to(move)));
// }

// // we assume attacker is side
// // least valuable attacker - we assume that attacker side is 'side'
// int Board::lva(int to_sq) const {
//     if(board->side == BLACK) {
// 		if((mask_sq(to_sq) & ~COL_0) && (to_sq + 7) >= 0 && (to_sq + 7) < 64 && get_piece(to_sq + 7) == BLACK_PAWN)
//         	return to_sq + 7;
//         if((mask_sq(to_sq) & ~COL_7) && (to_sq + 9) >= 0 && (to_sq + 9) < 64 && get_piece(to_sq + 9) == BLACK_PAWN)
//         	return to_sq + 9; 
//     } else {
//         if((mask_sq(to_sq) & ~COL_0) && (to_sq - 9) >= 0 && (to_sq - 9) < 64 && get_piece(to_sq - 9) == WHITE_PAWN)
//         	return to_sq - 9; 
//         if((mask_sq(to_sq) & ~COL_7) && (to_sq - 7) >= 0 && (to_sq - 7) < 64 && get_piece(to_sq - 7) == WHITE_PAWN) 
//         	return to_sq - 7;
//     }
// 	if(knight_attacks[to_sq] & get_knight_mask(board->side))
// 		return lsb(knight_attacks[to_sq] & get_knight_mask(board->side));
//     if(Rmagic(to_sq, get_all_mask()) & (get_rook_mask(board->side) | get_queen_mask(board->side)))
//     	return lsb(Rmagic(to_sq, get_all_mask()) & (get_rook_mask(board->side) | get_queen_mask(board->side)));
//     if(Bmagic(to_sq, get_all_mask()) & (get_bishop_mask(board->side) | get_queen_mask(board->side)))
//     	return lsb(Bmagic(to_sq, get_all_mask()) & (get_bishop_mask(board->side) | get_queen_mask(board->side)));
//     return -1;
// }

// int Board::fast_see(Move move) const {
// 	int from_sq, to_sq, piece_at_to, depth, side;
// 	uint64_t attacker_mask, occ_mask, _diagonal_mask;
// 	std::vector<int> score(16, 0);

// 	to_sq = get_to(move);
// 	from_sq = get_from(move);
// 	depth = 0;
// 	side = board->side;
// 	piece_at_to = get_piece(to_sq);
// 	attacker_mask = get_attackers(to_sq, side) | get_attackers(to_sq, xside);	

// 	while(from_sq != -1) {
// 		score[depth + 1] = piece_value[piece_at_to] - score[depth];
// 		depth++;
// 		piece_at_to = get_piece(from_sq);
// 		attacker_mask ^= mask_sq(from_sq);
// 		side = !side;

// 		// check xrays
// 		if(move_is_diagonal(move)) {
// 			if(from_sq > to_sq) { // north
// 				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][NORTHEAST];
// 				else _diagonal_mask = diagonal_mask[from_sq][NORTHWEST];
// 			} else { // south
// 				if(col(from_sq) < col(to_sq)) _diagonal_mask = diagonal_mask[from_sq][SOUTHEAST];
// 				else _diagonal_mask = diagonal_mask[from_sq][SOUTHWEST];
// 			}

// 			attacker_mask |= Bmagic(from_sq, board->occ_mask) & _diagonal_mask
// 			& (get_queen_mask(side) | get_bishop_mask(side) | get_queen_mask(!side) | get_bishop_mask(!side)); 
// 		} else if(move_is_straight(move)) {
// 			if(row(from_sq) == row(to_sq)) {
// 				attacker_mask |= Rmagic(from_sq, board->occ_mask) & straight_mask[from_sq][from_sq < to_sq ? EAST : WEST]
// 				& (get_queen_mask(side) | get_rook_mask(side) | get_queen_mask(!side) | get_rook_mask(!side));
// 			} else {
// 				attacker_mask |= Rmagic(from_sq, board->occ_mask) & straight_mask[from_sq][from_sq < to_sq ? SOUTH : NORTH]
// 				& (get_queen_mask(side) | get_rook_mask(side) | get_queen_mask(!side) | get_rook_mask(!side));
// 			}
// 		}

// 		// if next_lva returns -1 it means that there are no more attackers from current attacker side 
// 		from_sq = next_lva(attacker_mask, side);
// 		// the king has put himself in check
// 		if(piece_at_to == WHITE_KING || piece_at_to == BLACK_KING) {
// 			depth--;
// 			from_sq = -1;
// 		}
// 	}

// 	score[0] = 10000; // we want to force the capture at the root
// 	score[depth + 1] = -score[depth];

// 	while(depth > 0) {
// 		// (Kind of) minimax optimization. At each node you can choose between not making the capture
// 		// or making the capture and assuming that the other side will play optimally after that
// 		score[depth] = std::max(-score[depth - 1], -score[depth + 1]);
// 		depth--;
// 	}

// 	return score[1];
// }

// int Board::slow_see(Move move, bool root) {
// 	const int piece_from = get_piece(get_from(move));
// 	const int piece_captured = get_piece(get_to(move));
// 	const int from_sq = get_from(move);
// 	const int to_sq = get_to(move);

// 	// making the capture
// 	// setting the 'to' square
// 	bits[piece_from] ^= mask_sq(from_sq);
// 	bits[piece_from] ^= mask_sq(to_sq);
// 	bits[piece_captured] ^= mask_sq(to_sq); 	
// 	side = xside;
// 	xside = !xside;

// 	const int captured_piece_value = piece_value[piece_captured];
// 	const int lva_square = lva(to_sq);

// 	int score;

// 	if(lva_square == -1) {
// 		score = captured_piece_value;
// 	} else {
// 		score = captured_piece_value - slow_see(Move(lva_square, get_to(move), CAPTURE_MOVE));
// 	}

// 	// unmaking move 
// 	// clearing the 'to' square
// 	bits[piece_from] ^= mask_sq(from_sq);
// 	bits[piece_from] ^= mask_sq(to_sq);
// 	bits[piece_captured] ^= mask_sq(to_sq); 	
// 	side = xside;
// 	xside = !xside;

// 	return score;
// }