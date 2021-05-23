#include <sys/timeb.h>
#include "defs.h"
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

static const uint64_t new_zobrist_castling[16] = {0};

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

void Board::new_make_move(const Move move, UndoData& undo_data) {
	int from_sq, to_sq, flag, piece, side_piece, adjacent;

	from_sq = get_from(move);
	to_sq = get_to(move);
	flag = get_flag(move);
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

	key ^= new_zobrist_castling[castling_flag];
	castling_flag &= castling_bitmasks[from_sq] & castling_bitmasks[to_sq];
	key ^= new_zobrist_castling[castling_flag];

	switch(flag) {
		case NULL_MOVE:
			break;
		case QUIET_MOVE:
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
		case CAPTURE_MOVE:
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
		case CASTLING_MOVE:
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
		case ENPASSANT_MOVE:
			adjacent = (row(from_sq) << 3) | col(to_sq);
			assert(adjacent == (row(from_sq) * 8 + col(to_sq)));
			undo_data.captured_piece = PAWN + (side ? 0 : 6);
			b_pst[side] += pst[piece][to_sq] - pst[piece][from_sq];
			b_pst[xside] -= pst[piece_at[adjacent]][to_sq];
			b_mat[xside] -= piece_value[piece_at[adjacent]];
			key ^= zobrist_pieces[side_piece][from_sq] ^ zobrist_pieces[side_piece][to_sq] ^ zobrist_pieces[undo_data.captured_piece][adjacent];
			color_at[to_sq] = side;
			piece_at[to_sq] = PAWN;
			color_at[from_sq] = piece_at[from_sq] = color_at[adjacent] = piece_at[adjacent] = EMPTY;
			bits[side_piece] ^= mask_sq(from_sq) | mask_sq(to_sq);
			bits[undo_data.captured_piece] ^= mask_sq(adjacent);
			occ_mask ^= mask_sq(from_sq) | mask_sq(to_sq) | mask_sq(adjacent);
			break;
		case KNIGHT_PROMOTION: case BISHOP_PROMOTION: case ROOK_PROMOTION: case QUEEN_PROMOTION:
			int promotion_piece = KNIGHT + flag - KNIGHT_PROMOTION; 
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

	fifty_move_ply++;

	if(side)
		move_count++;

	if(piece == PAWN || flag == CAPTURE_MOVE)
		fifty_move_ply = 0;

	xside = side;
	side = !side;
	key ^= zobrist_side[side] ^ zobrist_side[xside];

    keys.push_back(key);

    king_attackers = get_attackers(lsb(get_king_mask(side)), xside, this);
}
