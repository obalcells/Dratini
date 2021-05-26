#include <vector>
#include <cassert>
#include "board.h"
#include "magicmoves.h"
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

void Board::take_back(const UndoData& undo_data) {
    side = xside;
	xside = !xside;

	fifty_move_ply = undo_data.fifty_move_ply;

	if(side == BLACK) {
		move_count--;
	}

	assert(move_count >= 0);

    assert(!keys.empty());
    keys.pop_back();
    key = keys.back();

    enpassant = undo_data.enpassant;

	castling_flag = undo_data.castling_flag;

    switch(get_flag(undo_data.move)) {
        case NULL_MOVE:
            break;
        case QUIET_MOVE: {
            clear_square(get_to(undo_data.move), undo_data.moved_piece);
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}
        case CAPTURE_MOVE: {
            clear_square(get_to(undo_data.move), undo_data.moved_piece);
            set_square(get_to(undo_data.move), undo_data.captured_piece);
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}
        case ENPASSANT_MOVE: {
            const int adjacent = row(get_from(undo_data.move)) * 8 + col(get_to(undo_data.move));
            clear_square(get_to(undo_data.move), undo_data.moved_piece);
            set_square(adjacent, undo_data.captured_piece);
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}		
        case CASTLING_MOVE: {
            clear_square(get_to(undo_data.move), undo_data.moved_piece);
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            switch(get_to(undo_data.move)) {
                case C1: { 
                    clear_square(D1, WHITE_ROOK);
                    set_square(A1, WHITE_ROOK);
                    break;
				}
                case G1: {
					assert(get_piece(F1) == WHITE_ROOK);
                    clear_square(F1, WHITE_ROOK);
                    set_square(H1, WHITE_ROOK);
                    break;
				}
                case C8: {
					assert(get_piece(D8) == BLACK_ROOK);
                    clear_square(D8, BLACK_ROOK);
                    set_square(A8, BLACK_ROOK);
                    break;
				}
                case G8: {
					assert(get_piece(F8) == BLACK_ROOK);
                    clear_square(F8, BLACK_ROOK);
                    set_square(H8, BLACK_ROOK);
                    break;
				}
            }
            break;
		}
        case QUEEN_PROMOTION: {
            clear_square(get_to(undo_data.move), QUEEN + (side == WHITE ? 0 : 6));
            if(undo_data.captured_piece != EMPTY) {
                set_square(get_to(undo_data.move), undo_data.captured_piece);
            }
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}
        case ROOK_PROMOTION: {
            clear_square(get_to(undo_data.move), ROOK + (side == WHITE ? 0 : 6));
            if(undo_data.captured_piece != EMPTY) {
                set_square(get_to(undo_data.move), undo_data.captured_piece);
            }
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}
        case KNIGHT_PROMOTION: {
            clear_square(get_to(undo_data.move), KNIGHT + (side == WHITE ? 0 : 6));
			assert(piece_at[get_to(undo_data.move)] == EMPTY && color_at[get_to(undo_data.move)]);
            if(undo_data.captured_piece != EMPTY) {
                set_square(get_to(undo_data.move), undo_data.captured_piece);
            }
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}
        case BISHOP_PROMOTION: {
            clear_square(get_to(undo_data.move), BISHOP + (side == WHITE ? 0 : 6));
            if(undo_data.captured_piece != EMPTY) {
                set_square(get_to(undo_data.move), undo_data.captured_piece);
            }
            set_square(get_from(undo_data.move), undo_data.moved_piece);
            break;
		}
    }

	// we can store this in UndoData?
    king_attackers = get_attackers(lsb(get_king_mask(side)), xside, this);
}

void Board::make_move(const Move move, UndoData& undo_data) {
	int from_sq = get_from(move);
	int to_sq = get_to(move);
	int flag = get_flag(move);
	int piece = get_piece(from_sq);

	undo_data.move = move;
	undo_data.enpassant = enpassant;
	undo_data.castling_flag = castling_flag;
	undo_data.moved_piece = piece;
	undo_data.captured_piece = EMPTY;
	undo_data.fifty_move_ply = fifty_move_ply;

	if(is_null(move)) {
		piece = -1;
		flag = -1;
	} else if(flag == QUIET_MOVE) {
		assert(get_piece(to_sq) == EMPTY);
		clear_square(from_sq, piece);
		set_square(to_sq, piece);
	} else if(flag == CAPTURE_MOVE) {
        undo_data.captured_piece = get_piece(to_sq);
		clear_square(to_sq, undo_data.captured_piece);
		clear_square(from_sq, piece);
		set_square(to_sq, piece);
	} else if(flag == CASTLING_MOVE) {
		int rook_from, rook_to;
		if(from_sq == E1 && to_sq == C1) { rook_from = A1; rook_to = D1; }
		else if(from_sq == E1 && to_sq == G1) { rook_from = H1; rook_to = F1; }
		else if(from_sq == E8 && to_sq == C8) { rook_from = A8; rook_to = D8; }
		else if(from_sq == E8 && to_sq == G8) { rook_from = H8; rook_to = F8; }
		else assert(false);
        undo_data.captured_piece = get_piece(rook_from);
		clear_square(from_sq, KING + (side == WHITE ? 0 : 6));
		clear_square(rook_from, ROOK + (side == WHITE ? 0 : 6));
		set_square(to_sq, KING + (side == WHITE ? 0 : 6));
		set_square(rook_to, ROOK + (side == WHITE ? 0 : 6));
	} else if(flag == ENPASSANT_MOVE) {
		int adjacent = row(from_sq) * 8 + col(to_sq);
        undo_data.captured_piece = get_piece(adjacent);
		clear_square(from_sq, piece);
		clear_square(adjacent, PAWN + (xside == WHITE ? 0 : 6));
		set_square(to_sq, piece);
	} else {
		int promotion_piece;
		switch(flag) {
			case KNIGHT_PROMOTION:
				promotion_piece = KNIGHT;
				break;
			case BISHOP_PROMOTION:
				promotion_piece = BISHOP;
				break;
			case ROOK_PROMOTION:
				promotion_piece = ROOK;
				break;
			case QUEEN_PROMOTION:
				promotion_piece = QUEEN;
				break;
			default:
				assert(false);
				break;
		}
		clear_square(from_sq, PAWN + (side == WHITE ? 0 : 6));
		if(get_piece(to_sq) != EMPTY) {
            undo_data.captured_piece = get_piece(to_sq);
			clear_square(to_sq, undo_data.captured_piece);
        }
		set_square(to_sq, promotion_piece + (side == WHITE ? 0 : 6));
	}

	if(piece == WHITE_PAWN || piece == BLACK_PAWN || flag == CAPTURE_MOVE) {
		fifty_move_ply = 0;
	}

	if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(from_sq - to_sq) == 16) {
		enpassant = col(from_sq);
	} else {
		enpassant = NO_ENPASSANT; // no column has enpassant
	}

	castling_flag &= castling_bitmasks[from_sq] & castling_bitmasks[to_sq];

	fifty_move_ply++;
	if(side == BLACK) {
		move_count++;
	}

	side = !side;
	xside = !xside;

    update_key(undo_data);
    keys.push_back(key);

    king_attackers = get_attackers(lsb(get_king_mask(side)), xside, this);
}

// returns false if move is invalid, otherwise it applies the move and returns true
// it calls make_move(Move) if the string represents a valid move
bool Board::make_move_from_str(const std::string& str_move) {
	if(
       (str_move[0] < 'a' || str_move[0] > 'h')
    || (str_move[1] < '1' || str_move[1] > '8')
    || (str_move[2] < 'a' || str_move[2] > 'h')
    || (str_move[3] < '1' || str_move[3] > '8')
	)
        return false;

    int from_sq = (str_move[1] - '1') * 8 + (str_move[0] - 'a');
    int to_sq = (str_move[3] - '1') * 8 + (str_move[2] - 'a');
    int flags = 0;
    int piece = get_piece(from_sq);

    if(piece == EMPTY) {
        return false;
    }

    if((piece == BLACK_PAWN || piece == WHITE_PAWN) && (row(to_sq) == 0 || row(to_sq) == 7)) {
        if((side == WHITE && row(to_sq) == 0) || (side == BLACK && row(to_sq) == 7)) {
			cout << "Side is " << int(side) << endl;
			cout << "Piece color is " << int(color_at[from_sq]) << endl;
			cout << "to_sq row is " << int(row(to_sq)) << endl;
            return false;
        }
		if(str_move[4] == ' ' || str_move[4] == '=') {
			if(str_move[5] == 'Q' || str_move[5] == 'q')     flags = QUEEN_PROMOTION;
			else if(str_move[5] == 'N' || str_move[5] == 'n') flags = KNIGHT_PROMOTION;
			else if(str_move[5] == 'R' || str_move[5] == 'r') flags = ROOK_PROMOTION;
			else if(str_move[5] == 'B' || str_move[5] == 'b') flags = BISHOP_PROMOTION;
			else assert(false);
		} else {
			if(str_move[4] == 'Q' || str_move[4] == 'q')     flags = QUEEN_PROMOTION;
			else if(str_move[4] == 'N' || str_move[4] == 'n') flags = KNIGHT_PROMOTION;
			else if(str_move[4] == 'R' || str_move[4] == 'r') flags = ROOK_PROMOTION;
			else if(str_move[4] == 'B' || str_move[4] == 'b') flags = BISHOP_PROMOTION;
			else assert(false);
		}
    } else if((piece == WHITE_KING || piece == BLACK_KING) && abs(col(from_sq) - col(to_sq)) == 2) {
        flags = CASTLING_MOVE;
    } else if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(col(from_sq) - col(to_sq)) == 1 && get_piece(to_sq) == EMPTY) {
        flags = ENPASSANT_MOVE;
    } else if(get_piece(to_sq) != EMPTY) {
        flags = CAPTURE_MOVE;
    } else {
        flags = QUIET_MOVE;
    }

    Move move = Move(from_sq, to_sq, flags);

    if(!move_valid(move)) {
        return false;
    }

	UndoData undo_data;
    make_move(move, undo_data);

    return true;
}

bool Board::check_pawn_move(const Move move) const {
    int from_sq = get_from(move), to_sq = get_to(move), flag = get_flag(move);

	switch(flag) {
		case ENPASSANT_MOVE: {
			if(!move_diagonal(move))
				return false;
			int adjacent = 8 * row(from_sq) + col(to_sq);
			// the enpassant flag doesn't match with column of square to
			// there is only one row we can move to if we are eating enpassant
			if(enpassant != col(adjacent)
			|| (side == WHITE && row(to_sq) != 5)
			|| (side == BLACK && row(to_sq) != 2))
				return false;
			// if this fails there is a bug regarding the enpassant flag
			assert(get_piece(adjacent) == WHITE_PAWN || get_piece(adjacent) == BLACK_PAWN);
			break;
		}
		case CAPTURE_MOVE: {
			// we have to eat diagonally
			// there must be an enemy piece at to square
			if(!move_diagonal(move) || color_at[to_sq] != xside)
				return false;
			break;
		}
		case QUIET_MOVE: {
			// it can only go one forward or two forward
			if(!((side == WHITE && to_sq == from_sq + 8)
			|| (side == WHITE && row(from_sq) == 1 && to_sq == from_sq + 16)
			|| (side == BLACK && to_sq == from_sq - 8)
			|| (side == BLACK && row(from_sq) == 6 && to_sq == from_sq - 16)))
				return false;
			// square in front can't be occupied
			if(mask_sq(from_sq + (side == WHITE ? 8 : (-8))) & occ_mask) // it should return 0 */
				return false;
			// if it does two-forward, that square can't be occupied 
			if((to_sq == from_sq + 16 || to_sq == from_sq - 16) && (mask_sq(to_sq) & occ_mask))
				return false;
			break;
		}
		default: {
			// flag must be promotion
			if(flag != QUEEN_PROMOTION
			&& flag != ROOK_PROMOTION
			&& flag != BISHOP_PROMOTION
			&& flag != KNIGHT_PROMOTION)
				return false;
			// to square must be at the last/first row
			if((side == WHITE && row(to_sq) != 7) || (side == BLACK && row(to_sq) != 0))
				return false;
			// it must be a valid pawn move
			int new_flag = (get_piece(to_sq) == EMPTY ? QUIET_MOVE : CAPTURE_MOVE);
			Move move_with_changed_flag = Move(from_sq, to_sq, new_flag); 
			return check_pawn_move(move_with_changed_flag);
		}
    }

    return true;
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

bool Board::move_valid(const Move move) {
	int from_sq = get_from(move);
	int to_sq = get_to(move);
	int flag = get_flag(move);

	if(from_sq < 0 || from_sq >= 64
	|| to_sq < 0 || to_sq >= 64
	|| (flag == QUIET_MOVE && get_piece(to_sq) != EMPTY)
	|| (flag == CAPTURE_MOVE && get_piece(to_sq) == EMPTY)) {
		return false;
	}

	int piece = get_piece(from_sq); 

	// trivial conditions that must be met
	if(from_sq == to_sq || piece == EMPTY || get_color(to_sq) == side || get_color(from_sq) != side) {
		return false;
    }

	if(flag == CASTLING_MOVE) {
		// this is the only place were we return true before the end of the function
		return castling_valid(move);
	} else if(piece == WHITE_PAWN || piece == BLACK_PAWN) {
		if(!check_pawn_move(move)) {
			return false;
        }
	} else {
		piece -= (side == BLACK ? 6 : 0);
		switch(piece) {
			case KING:
				if (!(mask_sq(to_sq) & king_attacks[from_sq]))
					return false;
				break;
			case KNIGHT:
				if (!(mask_sq(to_sq) & knight_attacks[from_sq]))
					return false;
				break;
			case BISHOP:
				if (!(mask_sq(to_sq) & Bmagic(from_sq, get_all_mask())))
					return false;
				break;
			case ROOK:
				if(!(mask_sq(to_sq) & Rmagic(from_sq, get_all_mask())))
					return false;
				break;
			case QUEEN:
				uint64_t occupation = get_all_mask();
				if(!(mask_sq(to_sq) & Bmagic(from_sq, occupation)) && !(mask_sq(to_sq) & Rmagic(from_sq, occupation))) {
					return false;
                }
				break;
		}
		piece += (side == BLACK ? 6 : 0);
	}

	// simulating make_move function
	int captured_piece;
	if(flag == ENPASSANT_MOVE) {
		int adjacent = 8 * row(from_sq) + col(to_sq);
		assert(get_piece(adjacent) == WHITE_PAWN || get_piece(adjacent) == BLACK_PAWN);
		uint64_t bb_before = get_pawn_mask(side);
		clear_square(adjacent, PAWN + (xside == WHITE ? 0 : 6));
        assert(bb_before == get_pawn_mask(side));
		assert(get_piece(to_sq) == EMPTY);
		set_square(to_sq, piece);
		assert(get_piece(from_sq) == WHITE_PAWN || get_piece(from_sq) == BLACK_PAWN);
		clear_square(from_sq, piece);
		assert(adjacent != to_sq && adjacent != from_sq);
	} else {
		captured_piece = get_piece(to_sq);
		clear_square(from_sq, piece);
		if(captured_piece != EMPTY) {
			clear_square(to_sq, captured_piece);
        }
		set_square(to_sq, piece);
	}

	const bool in_check_after_move = is_attacked(lsb(get_king_mask(side)));

	// reverse the modifications
	if(flag == ENPASSANT_MOVE) {
		const int adjacent = 8 * row(from_sq) + col(to_sq);
		clear_square(to_sq, piece);
		set_square(adjacent, PAWN + (xside == WHITE ? 0 : 6));
		set_square(from_sq, piece);
	} else {
		clear_square(to_sq, piece);
		if(captured_piece != EMPTY) {
			set_square(to_sq, captured_piece);
        }
		set_square(from_sq, piece);
	}

	if(in_check_after_move) {
		return false;
	}

	return true;
}
