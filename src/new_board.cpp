#include <random>
#include <vector>
#include <iostream>

#include "magicmoves.h"
#include "new_board.h"
#include "new_defs.h"
#include "defs.h"

std::vector<std::vector<uint64_t> > zobrist_pieces;
std::vector<uint64_t> zobrist_castling;
std::vector<uint64_t> zobrist_enpassant;
std::vector<uint64_t> zobrist_side;
std::vector<std::vector<uint64_t> > pawn_attacks;
std::vector<uint64_t> knight_attacks;
std::vector<uint64_t> king_attacks;
std::vector<uint64_t> castling_mask;
const char piece_char[12] = { 
    'P', 'N', 'B', 'R', 'Q', 'K',
    'p', 'n', 'b', 'r', 'q', 'k',
};

/* initializes zobrist hashes and bitboard tables */
void BitBoard::init_data() {
    if(required_data_initialized)
        return;

	initmagicmoves();

    zobrist_pieces.resize(12);
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
        zobrist_pieces[piece].resize(64);
        for(int sq = 0; sq < 64; sq++)
            zobrist_pieces[piece][sq] = get_random_64();
    }
    
    zobrist_castling.resize(4);
    for(int castling_flag = 0; castling_flag < 4; castling_flag++)
        zobrist_castling[castling_flag] = get_random_64();

    zobrist_enpassant.resize(9);
    for(int col = 0; col < 8; col++)
        zobrist_enpassant[col] = get_random_64();
    /* when there is no enpassant column */
    zobrist_enpassant[NO_ENPASSANT] = 0;

    zobrist_side.resize(2);
    for(int _side = WHITE; _side <= BLACK; _side++)
        zobrist_side[_side] = get_random_64();

    /* pawn tables */
    pawn_attacks.resize(2);
    pawn_attacks[WHITE].assign(64, 0);
    pawn_attacks[BLACK].assign(64, 0);
    for(int sq = 0; sq < 64; sq++) {
        int _row = row(sq), _col = col(sq);
		if(_row > 0 && _col > 0)
			pawn_attacks[WHITE][sq] |= mask_sq(sq - 9);
		if(_row > 0 && _col < 7)
			pawn_attacks[WHITE][sq] |= mask_sq(sq - 7);
		if(_row < 7 && _col > 0)
			pawn_attacks[BLACK][sq] |= mask_sq(sq + 7);
		if(_row < 7 && _col < 7)
			pawn_attacks[BLACK][sq] |= mask_sq(sq + 9);
    }

    /* knight tables */
    knight_attacks.clear();
    knight_attacks.resize(64);
    for(int sq = 0; sq < 64; sq++) {
        int _row = row(sq), _col = col(sq);
		knight_attacks[sq] = 0;
        /* one up, two left */
        if(_col > 1 && _row < 7)
            knight_attacks[sq] |= mask_sq(sq + 8 - 2);
        /* two up, one left */
        if(_col > 0 && _row < 6)
            knight_attacks[sq] |= mask_sq(sq + 16 - 1);
        /* two up, one right */
        if(_col < 7 && _row < 6)
            knight_attacks[sq] |= mask_sq(sq + 16 + 1);
        /* one up, two right */
        if(_col < 6 && _row < 7)
            knight_attacks[sq] |= mask_sq(sq + 8 + 2);
        /* one down, two right */
        if(_col < 6 && _row > 0)
            knight_attacks[sq] |= mask_sq(sq - 8 + 2);
        /* two down, one right */
        if(_col < 7 && _row > 1)
            knight_attacks[sq] |= mask_sq(sq - 16 + 1);
        /* two down, one left */
        if(_col > 0 && _row > 1)
            knight_attacks[sq] |= mask_sq(sq - 16 - 1);
        /* one down, two left */
        if(_col > 1 && _row > 0)
            knight_attacks[sq] |= mask_sq(sq - 8 - 2);
    }

    /* king tables */
    king_attacks.clear();
    king_attacks.resize(64);
    for(int sq = 0; sq < 64; sq++) {
        int _row = row(sq), _col = col(sq);
        /* one left */
        if(_col > 0)
            king_attacks[sq] |= mask_sq(sq - 1);
        /* one left, one up */
        if(_col > 0 && _row < 7)
            king_attacks[sq] |= mask_sq(sq + 7);
        /* one up */
        if(_row < 7)
            king_attacks[sq] |= mask_sq(sq + 8);
        /* one right, one up */
        if(_col < 7 && _row < 7)
            king_attacks[sq] |= mask_sq(sq + 9);
        /* one right */
        if(_col < 7)
            king_attacks[sq] |= mask_sq(sq + 1);
        /* one right, one down */
        if(_col < 7 && _row > 0)
            king_attacks[sq] |= mask_sq(sq - 8 + 1);
        /* one down */
        if(_row > 0)
            king_attacks[sq] |= mask_sq(sq - 8);
        /* one left, one down */
        if(_col > 0 && _row > 0)
            king_attacks[sq] |= mask_sq(sq - 8 - 1);
    }

	castling_mask.resize(4);
	castling_mask[WHITE_QUEEN_SIDE] = mask_sq(B1) | mask_sq(C1) | mask_sq(D1);
	castling_mask[WHITE_KING_SIDE] = mask_sq(F1) | mask_sq(G1);
	castling_mask[BLACK_QUEEN_SIDE] = mask_sq(B8) | mask_sq(C8) | mask_sq(D8);
	castling_mask[BLACK_KING_SIDE] = mask_sq(F8) | mask_sq(G8);

    required_data_initialized = true;
}

void BitBoard::clear_board() {
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
		bits[piece] &= 0;
}

void BitBoard::set_from_fen(const std::string& fen) {
	/* https://www.chessprogramming.org/Forsyth-Edwards_Notation */
	clear_board();
	int index = 0, _row = 7;
	while(_row >= 0) {
		int _col = 0;
		while(_col < 8) {
			assert(index < (int)fen.size());
			char _char = fen[index++];
			if(_char == '/') {

			} else if(_char >= '1' && _char <= '8') {
				_col += _char - '0';
			} else {
				const int sq = _row * 8 + _col;
				switch(_char) {
				case 'P':
					set_square(sq, WHITE_PAWN);
					break;
				case 'N':
					set_square(sq, WHITE_KNIGHT);
					break;
				case 'B':
					set_square(sq, WHITE_BISHOP);
					break;
				case 'R':
					set_square(sq, WHITE_ROOK);
					break;
				case 'Q':
					set_square(sq, WHITE_QUEEN);
					break;
				case 'K':
					set_square(sq, WHITE_KING);
					break;
				case 'p':
					set_square(sq, BLACK_PAWN);
					break;
				case 'n':
					set_square(sq, BLACK_KNIGHT);
					break;
				case 'b':
					set_square(sq, BLACK_BISHOP);
					break;
				case 'r':
					set_square(sq, BLACK_ROOK);
					break;
				case 'q':
					set_square(sq, BLACK_QUEEN);
					break;
				case 'k':
					set_square(sq, BLACK_KING);
					break;
				default:
					std::cout << "Invalid fen string (1)" << endl;
					clear_board();
					return;
				}
				_col++;
			}
		}
		_row--;
		if(!(fen[index] == '/' || (_row == -1 && fen[index] == ' '))) {
			std::cout << "Invalid fen string (2)" << endl;
		}
		index++;
	}
	/* side to move */
	if(fen[index] == 'w') {
		side = WHITE;
		xside = BLACK;
	} else if(fen[index] == 'b') {
		side = BLACK;
		xside = WHITE;
	} else {
		std::cout << "Invalid fen string (3)" << endl;
		clear_board();
		return;
	}
	/* space */
	if(fen[index] == ' ') {
		std::cout << "Invalid fen string (4)" << endl;
		clear_board();
		return;
	}
	index++;
	if(fen[index] != ' ') {
		std::cout << "Invalid fen string (5)" << endl;
		clear_board();
		return;
	}
	index++;
	/* castling */
	if(fen[index] == '-') {
		castling_rights[WHITE_QUEEN_SIDE] = false;
		castling_rights[WHITE_KING_SIDE] = false;
		castling_rights[BLACK_QUEEN_SIDE] = false;
		castling_rights[BLACK_KING_SIDE] = false;
		index++;
	} else {
		while(fen[index] != ' ') {
			switch(fen[index++]) {
				case 'K':
					castling_rights[BLACK_KING_SIDE] = true;
					break;
				case 'Q':
					castling_rights[BLACK_QUEEN_SIDE] = true;
					break;
				case 'k':
					castling_rights[WHITE_KING_SIDE] = true;
					break;
				case 'q':
					castling_rights[WHITE_QUEEN_SIDE] = true;
					break;
				default:
					std::cerr << "Invalid fen string (5)" << endl;
					clear_board();
					return;
			}
		}
	}
	/* space */
	if(fen[index] != ' ') {
		std::cout << "Invalid fen string (6)" << endl;
		clear_board();
		return;
	}
	index++;
	/* enpassant square */
	if(fen[index] == '-') {
		set_enpassant(NO_ENPASSANT);
		index++;
	} else {
		char char_col = fen[index++];
		char char_row = fen[index++];
		if(!(char_col >= 'a' && char_col <= 'h')) {
			std::cout << "Invalid fen string (7)" << endl;
			clear_board();
			return;
		}
		int _col = char_col - 'a';
		set_enpassant(_col);
	}
	/* space */
	if(fen[index] != ' ') {
		std::cout << "Invalid fen string (8)" << endl;
		clear_board();
		return;
	}
	index++;
	std::string tmp_str = "";
	/* fifty move counter */
	while(fen[index] != ' ') {
		if(!(fen[index] >= '0' && fen[index] <= '9')) {
			std::cout << "Invalid fen string (9)" << endl;
			clear_board();
			return;
		}
		tmp_str += fen[index++];
	}
	try {
		fifty_move_ply = stoi(tmp_str);
	} catch(char* exception_name) {
		std::cout << "Invalid fen string (10)" << endl;
		clear_board();
		return;
	}
	/* space */
	if(fen[index] != ' ') {
		std::cout << "Invalid fen string (11)" << endl;
		clear_board();
		return;
	}
	index++;
	tmp_str.clear();
	/* move counter */
	while(index < (int)fen.size() && fen[index] != ' ') {
		if(!(fen[index] >= '0' && fen[index] <= '9')) {
			std::cout << "Invalid fen string (12)" << endl;
			clear_board();
			return;
		}
		tmp_str += fen[index++];
	}
	try {
		move_count = stoi(tmp_str);
	} catch(char* exception_name) {
		std::cout << "Invalid fen string (13)" << endl;
		clear_board();
		return;
	}
	key = calculate_key();
}

/* the piece in this case ranges from 0 to 5, so we need the _side parameter */
void BitBoard::set_square(int sq, int non_side_piece, bool _side) {
    bits[non_side_piece + (_side == BLACK ? 6 : 0)] |= mask_sq(sq);
}

/* the piece in this case ranges from 0 to 11 */
void BitBoard::set_square(int sq, int piece) {
    bits[piece] |= mask_sq(sq);
}

void BitBoard::set_enpassant(int col) {
	enpassant = uint8_t(col);
}

/* the piece ranges from 0 to 5 */
void BitBoard::clear_square(int sq, int non_side_piece, bool _side) {
    // assert(bits[non_side_piece + (_side == BLACK ? 6 : 0)] & mask_sq(sq));
    bits[non_side_piece + (_side == BLACK ? 6 : 0)] ^= mask_sq(sq);
}

/* the piece ranges from 0 to 11 */
void BitBoard::clear_square(int sq, int piece) {
    // assert(bits[piece] & mask_sq(sq));
    bits[piece] ^= mask_sq(sq);
}

bool BitBoard::is_empty(int sq) {
	const uint64_t mask = mask_sq(sq);
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
		if(mask & bits[piece])
			return false;
	}
	return true;
}

uint64_t BitBoard::get_all_mask() const {
	uint64_t mask = 0;
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
		mask |= bits[piece];
	return mask;
}

/* returns the mask of all white/black pieces */
uint64_t BitBoard::get_side_mask(bool _side) const {
	uint64_t mask = 0;
	if(_side == WHITE) {
		for(int piece = WHITE_PAWN; piece <= WHITE_KING; piece++) {
			mask |= bits[piece];
		}
	} else {
		for(int piece = BLACK_PAWN; piece <= BLACK_KING; piece++) {
			mask |= bits[piece];
		}
	}
	return mask;
}

uint64_t BitBoard::get_pawn_mask(bool _side) const {
	if(_side == WHITE)
		return bits[WHITE_PAWN];
	return bits[BLACK_PAWN];
}

uint64_t BitBoard::get_knight_mask(bool _side) const {
	if(_side == WHITE)
		return bits[WHITE_KNIGHT];
	return bits[BLACK_KNIGHT];
}

uint64_t BitBoard::get_bishop_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_BISHOP];
	return bits[BLACK_BISHOP];
}

uint64_t BitBoard::get_rook_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_ROOK];
	return bits[BLACK_ROOK];
}

uint64_t BitBoard::get_queen_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_QUEEN];
	return bits[BLACK_QUEEN];
}

uint64_t BitBoard::get_king_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_KING];
	return bits[BLACK_KING];
}

int BitBoard::get_piece(int sq) const {
    uint64_t mask = mask_sq(sq);
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        if(mask & bits[piece])
            return piece;
    return NEW_EMPTY;
}

/* returns whether a piece at square is WHITE, BLACK or EMPTY */
int BitBoard::get_color(int sq) const {
    uint64_t mask = mask_sq(sq);
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        if(mask & bits[piece])
            return ((piece >= BLACK_PAWN) ? BLACK : WHITE); 
    return NEW_EMPTY;
}

void BitBoard::update_castling_rights(const NewMove& move) {
    const int piece = get_piece(move.get_to());

	if(piece == WHITE_KING) {
		castling_rights[WHITE_QUEEN_SIDE] = false;
		castling_rights[WHITE_KING_SIDE] = false;
    } else if(piece == BLACK_KING) {
		castling_rights[BLACK_QUEEN_SIDE] = false;
		castling_rights[BLACK_KING_SIDE] = false;
	}

	if(castling_rights[WHITE_QUEEN_SIDE] && get_piece(A1) != WHITE_ROOK)
		castling_rights[WHITE_QUEEN_SIDE] = false;
	else if(castling_rights[WHITE_KING_SIDE] && get_piece(H1) != WHITE_ROOK)
		castling_rights[WHITE_KING_SIDE] = false;

	if(castling_rights[BLACK_QUEEN_SIDE] && get_piece(A8) != BLACK_ROOK)
		castling_rights[BLACK_QUEEN_SIDE] = false;
	else if(castling_rights[BLACK_KING_SIDE] && get_piece(H8) != BLACK_ROOK)
		castling_rights[BLACK_KING_SIDE] = false;
}

uint64_t BitBoard::calculate_key() const {
	uint64_t new_key = 0;

	new_key ^= zobrist_enpassant[enpassant];

    for(int castling_type = 0; castling_type < 4; castling_type++)
		if(castling_rights[castling_type])
			new_key ^= zobrist_castling[castling_type];

	int piece;
	for(int sq = 0; sq < 64; sq++) {
		piece = get_piece(sq);
		if(piece != NEW_EMPTY)
			new_key ^= zobrist_pieces[piece][sq];   
	}

	new_key ^= zobrist_side[side];

	return new_key;
}

void BitBoard::update_key(const BitBoard& board_before, const NewMove& move) {
    if(move.is_move_null()) {
		key ^= zobrist_side[side];
		key ^= zobrist_side[xside];
        return;
	}

    if(board_before.enpassant != enpassant) {
        key ^= zobrist_enpassant[board_before.enpassant];
        key ^= zobrist_enpassant[enpassant];
    }
    
    for(int castling_type = 0; castling_type < 4; castling_type++) {
        if(board_before.castling_rights[castling_type] != castling_rights[castling_type])
            key ^= zobrist_castling[castling_type];
	}
        
    const int from_sq = move.get_from();
    const int to_sq   = move.get_to();
    const int flag    = move.get_flag();
	const int piece   = board_before.get_piece(from_sq);

    if(flag == QUIET_MOVE) {
		key ^= zobrist_pieces[piece][from_sq]; 
		key ^= zobrist_pieces[piece][to_sq];
    } else if(flag == CASTLING_MOVE) {
        int rook_from = -1, rook_to = -1;
        if(from_sq == E1 && to_sq == C1) { rook_from = A1; rook_to = D1; } 
        else if(from_sq == E1 && to_sq == G1) { rook_from = H1; rook_to = F1; } 
        else if(from_sq == E8 && to_sq == C8) { rook_from = A8; rook_to = D8; }
        else if(from_sq == E8 && to_sq == G8) { rook_from = H8; rook_to = F8; }
		else assert(false);
        key ^= zobrist_pieces[(side == WHITE ? BLACK_ROOK : WHITE_ROOK)][rook_from];
        key ^= zobrist_pieces[(side == WHITE ? BLACK_ROOK : WHITE_ROOK)][rook_to];
		key ^= zobrist_pieces[piece][from_sq]; 
		key ^= zobrist_pieces[piece][to_sq];
    } else if(flag == ENPASSANT_MOVE) {
        const int adjacent = row(from_sq) * 8 + col(to_sq);
        key ^= zobrist_pieces[(side == WHITE ? WHITE_PAWN : BLACK_PAWN)][adjacent];
		key ^= zobrist_pieces[piece][from_sq]; 
		key ^= zobrist_pieces[piece][to_sq];
    } else if(flag == CAPTURE_MOVE) {
        key ^= zobrist_pieces[board_before.get_piece(to_sq)][to_sq];
		key ^= zobrist_pieces[piece][from_sq]; 
		key ^= zobrist_pieces[piece][to_sq];
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
		}
		const int captured_piece = board_before.get_piece(to_sq);
		if(captured_piece != NEW_EMPTY) {
			key ^= zobrist_pieces[captured_piece][to_sq];
		}
        key ^= zobrist_pieces[(side == WHITE ? BLACK_PAWN : WHITE_PAWN)][from_sq];
        key ^= zobrist_pieces[promotion_piece + (side == BLACK ? 0 : 6)][to_sq];
    }       

	key ^= zobrist_side[side];
    key ^= zobrist_side[xside];
}

bool BitBoard::is_attacked(const int sq) const {
    /* Version 1 (it works) */
    if(knight_attacks[sq] & get_knight_mask(xside))
        return true;

    if(king_attacks[sq] & get_king_mask(xside))
        return true;
	
    if(pawn_attacks[xside][sq] & get_pawn_mask(xside))
        return true;

    const uint64_t occupation = get_all_mask();

    const uint64_t bishop_moves = Bmagic(sq, occupation);

    if((bishop_moves & get_bishop_mask(xside)) || (bishop_moves & get_queen_mask(xside))) {
        return true;
    }

    const uint64_t rook_moves = Rmagic(sq, occupation);
    if((rook_moves & get_rook_mask(xside)) || (rook_moves & get_queen_mask(xside)))
        return true;

	return false;

    /* Version 2 (not tested, but it might be faster) */
    /*
    return 
    	!(knight_attacks[sq] & get_knight_mask(xside)) &&
    	!(king_attacks[sq] & get_king_mask(xside) &&
    	!(pawn_attacks[xside][sq] & get_pawn_mask(xside)) &&
		!(Bmagic(sq, get_all_mask()) & (get_bishop_mask(xside) | get_queen_mask(xside))) && 
		!(Rmagic(sq, get_all_mask()) & (get_rook_mask(xside) | get_queen_mask(xside)));
	*/
}

bool BitBoard::in_check() const {
	if(side == WHITE)
		return is_attacked(lsb(bits[WHITE_KING]));
	else
		return is_attacked(lsb(bits[BLACK_KING]));
}

bool BitBoard::castling_valid(const NewMove& move) const {
    int from_sq = move.get_from();
    int to_sq   = move.get_to();

	int castling_type;
	uint64_t all_mask = get_all_mask();

	/*
	std::cout << "Castling move is " << move_to_str(Move(from_sq, to_sq)) << endl;
	std::cout << "Board looks like:" << endl;
	print_board();
	std::cout << "All mask is:" << endl;
	print_bitboard(all_mask);
	std::cout << "Castling mask is:" << endl;
	print_bitboard(castling_mask[WHITE_QUEEN_SIDE]);
	std::cout << "Is valid? " << ((all_mask & castling_mask[WHITE_QUEEN_SIDE] == 0) ? "Yes" : "No") << endl;
	std::cout << RESET_COLOR;
	*/
	
	if(side == WHITE
	&& (from_sq == E1 && to_sq == C1)
	&& castling_rights[WHITE_QUEEN_SIDE]
	&& ((all_mask & castling_mask[WHITE_QUEEN_SIDE]) == 0)) {
		castling_type = WHITE_QUEEN_SIDE;
	} else if(side == WHITE
	&& (from_sq == E1 && to_sq == G1)
	&& (castling_rights[WHITE_KING_SIDE])
	&& ((all_mask & castling_mask[WHITE_KING_SIDE]) == 0)) {
		castling_type = WHITE_KING_SIDE;
	} else if(side == BLACK
	&& (from_sq == E8 && to_sq == C8)
	&& castling_rights[BLACK_QUEEN_SIDE]
	&& ((all_mask & castling_mask[BLACK_QUEEN_SIDE]) == 0)) {
		castling_type = BLACK_QUEEN_SIDE;
	} else if(side == BLACK
	&& (from_sq == E8 && to_sq == G8)
	&& castling_rights[BLACK_KING_SIDE]
	&& ((all_mask & castling_mask[BLACK_KING_SIDE]) == 0)) {
		castling_type = BLACK_KING_SIDE;
	} else {
		return false;
	}

    if(is_attacked(from_sq) || is_attacked(to_sq))
        return false;

	if(castling_type == WHITE_QUEEN_SIDE || castling_type == BLACK_QUEEN_SIDE) { 
		return !is_attacked(from_sq - 1);
    } else if(castling_type == WHITE_KING_SIDE || castling_type == BLACK_KING_SIDE) {
    	return !is_attacked(from_sq + 1);
    } else {
		assert(false);
		return false;
	}
}

/* it only works for pawns */
bool BitBoard::move_diagonal(const NewMove& move) const {
	int from_sq = move.get_from();
	int to_sq = move.get_to();
	if(side == WHITE) {
		if(to_sq != from_sq + 7 && to_sq != from_sq + 9)
			return false;
		/* wrong offset */
		if(col(to_sq) == 0 && to_sq != from_sq + 7)
			return false;
		/* wrong offset */
		if(col(to_sq) == 7 && to_sq != from_sq + 9)
			return false;
	} else {
		if(to_sq != from_sq - 7 && to_sq != from_sq - 9)
			return false;
		/* wrong offset */
		if(col(to_sq) == 0 && to_sq != from_sq - 9)
			return false;
		/* wrong offset */
		if(col(to_sq) == 7 && to_sq != from_sq - 7)
			return false;
	}
    return true;
}

bool BitBoard::check_pawn_move(const NewMove& move) const {
    int from_sq = move.get_from(); 
    int to_sq = move.get_to();
    int flag = move.get_flag();

    if(flag == ENPASSANT_MOVE) {
        if(!move_diagonal(move))
            return false;
        int adjacent = 8 * row(from_sq) + col(to_sq);
        /* the enpassant flag doesn't match with column of square to */
        if(enpassant != col(adjacent))
            return false;
        /* there is only one row we can move to if we are eating enpassant */
        if(side == WHITE && row(to_sq) != 5)
            return false;
        /* there is only one row we can move to if we are eating enpassant */
        if(side == BLACK && row(to_sq) != 2)
            return false;
        /* if this fails there is a bug regarding the enpassant flag */
        assert(get_piece(adjacent) == WHITE_PAWN || get_piece(adjacent) == BLACK_PAWN);
    } else if(flag == CAPTURE_MOVE) {
        /* we have to eat diagonally */
        if(!move_diagonal(move)) {
            return false;
		}
        /* there must be an enemy piece at to square */
        if(!(mask_sq(to_sq) & get_side_mask(xside)))
            return false;
    } else if(flag == QUIET_MOVE) {
        /* it can only go one forward or two forward */
		if((side == WHITE && to_sq == from_sq + 8)
		|| (side == WHITE && row(from_sq) == 1 && to_sq == from_sq + 16)
		|| (side == BLACK && to_sq == from_sq - 8)
		|| (side == BLACK && row(from_sq) == 6 && to_sq == from_sq - 16)) {
			/* good */	
		} else return false;
        /* we compute the mask already */
		uint64_t all_side_mask = get_all_mask();
        /* square in front can't be occupied */
        uint64_t one_forward = mask_sq(from_sq + (side == WHITE ? 8 : (-8)));
		if(one_forward & get_all_mask()) /* it should return 0 */
            return false;
        /* if it does two-forward, that square can't be occupied */
        if(to_sq == from_sq + 16 || to_sq == from_sq - 16) {
			if(mask_sq(to_sq) & all_side_mask) 
				return false;
        }
    } else {
        /* flag must be promotion */
        if(flag != QUEEN_PROMOTION
           && flag != ROOK_PROMOTION
           && flag != BISHOP_PROMOTION
           && flag != KNIGHT_PROMOTION)
            return false;
        /* to square must be at the last/first row */
        if(side == WHITE && row(to_sq) != 7)
            return false;
        /* to square must be at the last/first row */
        if(side == BLACK && row(to_sq) != 0)
            return false;
		/* it must be a valid pawn move */
		int new_flag = (get_piece(to_sq) == NEW_EMPTY ? QUIET_MOVE : CAPTURE_MOVE);
		NewMove move_with_changed_flag = NewMove(from_sq, to_sq, new_flag); 
		if(!check_pawn_move(move_with_changed_flag))
			return false;
    }
    return true;
}


void BitBoard::make_move(const NewMove& move) {
	int from_sq  = move.get_from();
	int to_sq    = move.get_to();
	int flag     = move.get_flag();
	int piece    = get_piece(from_sq);

	if(move.is_move_null()) {
		/* don't do anything */
	} else if(flag == QUIET_MOVE) {
		clear_square(from_sq, piece);
		set_square(to_sq, piece);
	} else if(flag == CAPTURE_MOVE) {
		clear_square(to_sq, get_piece(to_sq));
		clear_square(from_sq, piece);
		set_square(to_sq, piece);
	} else if(flag == CASTLING_MOVE) {
		int rook_from, rook_to;
		if(from_sq == E1 && to_sq == C1) { rook_from = A1; rook_to = D1; }
		else if(from_sq == E1 && to_sq == G1) { rook_from = H1; rook_to = F1; }
		else if(from_sq == E8 && to_sq == C8) { rook_from = A8; rook_to = D8; }
		else if(from_sq == E8 && to_sq == G8) { rook_from = H8; rook_to = F8; }
		else assert(false);
		clear_square(from_sq, KING, side);
		clear_square(rook_from, ROOK, side);
		set_square(to_sq, KING, side);
		set_square(rook_to, ROOK, side);
	} else if(flag == ENPASSANT_MOVE) {
		int adjacent = row(from_sq) * 8 + col(to_sq);
		clear_square(from_sq, piece);
		clear_square(adjacent, PAWN, xside);
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
		clear_square(from_sq, PAWN, side);
		if(get_piece(to_sq) != NEW_EMPTY)
			clear_square(to_sq, get_piece(to_sq));
		set_square(to_sq, promotion_piece, side); /* we have to pass the side too */
	}

	if(piece == WHITE_PAWN || piece == BLACK_PAWN || flag == CAPTURE_MOVE)
		fifty_move_ply = -1;

	if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(from_sq - to_sq) == 16) {
		set_enpassant(col(from_sq));
	} else {
		/* no column has enpassant */
		set_enpassant(8);
	}

	update_castling_rights(move);

	fifty_move_ply++;
	if(side == BLACK)
		move_count++;
	side = !side;
	xside = !side;
}

bool BitBoard::fast_move_valid(const NewMove& move) const {
	const int piece_from = get_piece(move.get_from());
	const int piece_to = get_piece(move.get_to());
	const int from_sq = move.get_from();
	const int to_sq = move.get_to();

	/*
	clear_square(from_sq, piece_from);
	if(piece_to != NEW_EMPTY)
		clear_square(to_sq, piece_to);
	set_square(to_sq, piece_from);
	*/

	// This code underneath is equivalent to just doing:
	// bool in_check_after_move = in_check();
	// the problem is that we want the function to be const 
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

		if(move.get_flag() == ENPASSANT_MOVE) {
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

bool BitBoard::move_valid(const NewMove& move) {
	// quick_check("Move valid beg");

	int from_sq = move.get_from();
	int to_sq   = move.get_to();
	int flag    = move.get_flag();

	/*
	if(move_to_str(Move(from_sq, to_sq)) == "e8g8") {
		std::cout << BLUE_COLOR;
		std::cout << "From sq " << from_sq << ", To sq " << to_sq << endl;
		std::cout << "Flag is " << flag << endl;
		print_board();
		std::cout << RESET_COLOR;
	}
	*/

	/* out of range */
	if(from_sq < 0 || from_sq >= 64 || to_sq < 0 || to_sq >= 64)
		return false;

	int piece = get_piece(from_sq); 

	/* trivial conditions that must be met */
	if(from_sq == to_sq || piece == NEW_EMPTY || get_color(to_sq) == side || get_color(from_sq) != side)
		return false;

	if(flag == CASTLING_MOVE) {
		/* this is the only place were we return true before the end of the function */
		return castling_valid(move);
	} else if(piece == WHITE_PAWN || piece == BLACK_PAWN) {
		if(!check_pawn_move(move))
			return false;
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
				if(!(mask_sq(to_sq) & Bmagic(from_sq, occupation)) && !(mask_sq(to_sq) & Rmagic(from_sq, occupation)))
					return false;
				break;
		}
		piece += (side == BLACK ? 6 : 0);
	}

	// quick_check("Post check");

	/* simulating make_move function */
	int captured_piece;
	if(flag == ENPASSANT_MOVE) {
		int adjacent = 8 * row(from_sq) + col(to_sq);
		assert(get_piece(adjacent) == WHITE_PAWN || get_piece(adjacent) == BLACK_PAWN);
		// quick_check("Enpassant 0");
		int my_side_pawn = (side == WHITE ? WHITE_PAWN : BLACK_PAWN);
		uint64_t bb_before = bits[my_side_pawn];
		clear_square(adjacent, PAWN, xside);
		assert(bb_before == bits[my_side_pawn]);
		// quick_check("Enpassant 1");
		assert(get_piece(to_sq) == NEW_EMPTY);
		set_square(to_sq, piece);
		// quick_check("Enpassant 2");
		assert(get_piece(from_sq) == WHITE_PAWN || get_piece(from_sq) == BLACK_PAWN);
		clear_square(from_sq, piece);
		assert(adjacent != to_sq && adjacent != from_sq);
		// quick_check("Enpassant");
	} else {
		captured_piece = get_piece(to_sq);
		if(captured_piece != NEW_EMPTY)
			clear_square(to_sq, captured_piece);
		set_square(to_sq, piece);
		clear_square(from_sq, piece);
	}

	// quick_check("Post editing");
	const bool in_check_after_move = is_attacked(lsb(bits[KING + (side == BLACK ? 6 : 0)])); 

	/* reverse the modifications */
	if(flag == ENPASSANT_MOVE) {
		const int adjacent = 8 * row(from_sq) + col(to_sq);
		set_square(adjacent, PAWN, xside);
		clear_square(to_sq, piece);
		set_square(from_sq, piece);
	} else {
		if(captured_piece != NEW_EMPTY)
			set_square(to_sq, captured_piece);
		clear_square(to_sq, piece);
		set_square(from_sq, piece);
	}

	// quick_check("End");

	if(in_check_after_move) {
		// std::cout << RED_COLOR << "Post-check wasn't passed" << RESET_COLOR << endl;
		return false;
	}

	// std::cout << GREEN_COLOR << "Checks passed. Move is valid." << RESET_COLOR << endl;
	return true;
}

void BitBoard::print_board() const {
	std::cout << endl;
	int i;
	for(i = 56; i >= 0;) {
		if(i % 8 == 0) std::cout << (i / 8) + 1 << "  ";
		std::cout << " ";
		if(get_piece(i) == NEW_EMPTY) {
			std::cout << '.';
		} else {
			std::cout << piece_char[get_piece(i)];
		}
		if((i + 1) % 8 == 0) {
			std::cout << '\n';
			i -= 15;
		} else {
			i++;
		}
	}
	std::cout << endl << endl << "   ";
	for(i = 0; i < 8; i++)
		std::cout << " " << char('a' + i);
	std::cout << endl;
	std::cout << "Move count: " << move_count << endl;
	std::cout << "Fifty move count: " << int(fifty_move_ply) << endl;
	if(enpassant == 8)
		std::cout << "Enpassant: None" << endl;
	else
		std::cout << "Enpassant: " << int(enpassant) << endl;
	std::cout << "Castling (WQ, WK, BQ, BK): ";
	for(i = 0; i < 4; i++)
		std::cout << (castling_rights[i] ? "Y " : "N ");
	std::cout << endl;
	std::cout << "Side: " << (side == WHITE ? "WHITE" : "BLACK") << endl;
	std::cout << "xSide: " << (xside == WHITE ? "WHITE" : "BLACK") << endl;
}

void BitBoard::print_bitboard(uint64_t bb) const {
	std::cout << endl;
	int i;
	for(i = 56; i >= 0;) {
		if(i % 8 == 0) std::cout << (i / 8) + 1 << "  ";
		std::cout << " ";
		if(bb & (uint64_t(1) << i)) {
			std::cout << 'X';
		} else {
			std::cout << '.';
		}
		// std::cout << RESET_COLOR;
		if((i + 1) % 8 == 0) {
			std::cout << '\n';
			i -= 15;
		} else {
			i++;
		}
	}
	std::cout << endl << endl << "   ";
	for(i = 0; i < 8; i++)
		std::cout << " " << char('a' + i);
	std::cout << endl;
}

void BitBoard::quick_check(const std::string& error_message) {
	for(int i = WHITE_PAWN; i <= BLACK_KING; i++) {
		for(int j = WHITE_PAWN; j <= BLACK_KING; j++) if(i != j) {
			if((bits[i] & bits[j]) > 0) {
				std::cerr << error_message << endl;
				std::cerr << i << " " << j << endl;
				if((bits[i] & bits[j]) == 0) {
					throw("Duplicated bits");
				}
			}
		}
	}
}

void BitBoard::same(const Position& other) {
	if((bool(other.castling & 1) && bool(other.castling & 4)) != castling_rights[WHITE_QUEEN_SIDE])
		throw("Castling is different");
	if((bool(other.castling & 2) && bool(other.castling & 4)) != castling_rights[WHITE_KING_SIDE])
		throw("Castling is different");
	if((bool(other.castling & 8) && bool(other.castling & 32)) != castling_rights[BLACK_QUEEN_SIDE])
		throw("Castling is different");
	if((bool(other.castling & 16) && bool(other.castling & 32)) != castling_rights[BLACK_KING_SIDE])
		throw("Castling is different");

	for(int i = 0; i < 64; i++) {
		if(other.piece[i] + (other.color[i] == BLACK ? 6 : 0) != get_piece(i)) {
			if(other.piece[i] == EMPTY) { 
				if(get_piece(i) != NEW_EMPTY) {
					throw("Boards are different");				
				}
			} else {
				if(other.piece[i] + (other.color[i] == BLACK ? 6 : 0) != get_piece(i)) {
					std::cerr << "Diff at " << pos_to_str(i) << " " << other.piece[i] + (other.color[i] == BLACK ? 6 : 0) << " " << get_piece(i) << endl;
					throw("Boards are different");
				}
			}
		}
	}
}
