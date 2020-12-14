#include <magicmoves.h>

/* pseudo-random number generator */
std::string str_seed = "Dratini is fast!";
std::seed_seq seed(str_seed.begin(), str_seed.end());
std::mt19937_64 rng(seed_seq);
std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

uint64_t get_random_64() {
	return dist(rng);
}

/* initializes zobrist hashes and bitboard tables */
void Board::init_data() {
	if(required_data_initialized)
		return;

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
	zobrist_enpassant[8] = 0;

	zobrist_side.resize(2);
	for(int _side = WHITE; _side <= BLACK; _side++)
		zobrist_side[_side] = get_random_64();

	/* pawn tables */
	pawn_attacks.resize(2);
	pawn_attacks[WHITE].resize(64);
	pawn_attacks[BLACK].resize(64);
	for(int sq = 0; sq < 64; sq++) {
		int _row = row(sq), _col = _col;
		if (_row < 7)
			pawn_attacks[WHITE][sq] |= mask_sq(sq + 8);
		if (_col > 0 && _row < 7)
			pawn_attacks[WHITE][sq] |= mask_sq(sq + 7);
		if (_col < 7 && _row < 7)
			pawn_attacks[WHITE][sq] |= mask_sq(sq + 9);
		if(_row > 0)
			pawn_attacks[BLACK][sq] |= mask_sq(sq - 8);
		if(_col > 0 && _row > 0)
			pawn_attacks[BLACK][sq] |= mask_sq(sq - 9);
		if(_col < 7 && _row > 0)
			pawn_attacks[BLACK][sq] |= mask_sq(sq - 7);
	}

	/* knight tables */
	knight_attacks.clear();
	knight_attacks.resize(64);
	for(int sq = 0; sq < 64; sq++) {
		int _row = row(sq), _col = col(sq);
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

	required_data_initialized = true;
}

uint64_t Board::mask_sq(int sq) {
	return (uint64_t(1) << sq);
}

void Board::clear_board() {
	for(int sq = 0; sq < 64; sq++)
		for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
			clear_square(sq, piece);
}

void Board::set_from_fen(std::string fen) {
	/* https://www.chessprogramming.org/Forsyth-Edwards_Notation */
	clear_board();
	int index = 0, _row = 0;
	while(_row < 8) {
		int _col = 0;
		while(_col < 8) {
			char _char = fen[index++];
			if(_char >= '1' && _char <= '8') {
				col += _char - '0';
			} else {
				_col++;
				const int sq = _row * 8 + _col;
				switch(_char) {
				case 'r':
					board.set_square(sq, WHITE_ROOK);
					break;
				case 'n':
					board.set_square(sq, WHITE_KNIGHT);
					break;
				case 'b':
					board.set_square(sq, WHITE_BISHOP);
					break;
				case 'q':
					board.set_square(sq, WHITE_QUEEN);
					break;
				case 'k':
					board.set_square(sq, WHITE_KING);
					break;
				case 'p':
					board.set_square(sq, WHITE_PAWN);
					break;
				case 'R':
					board.set_square(sq, BLACK_ROOK);
					break;
				case 'N':
					board.set_square(sq, BLACK_KNIGHT);
					break;
				case 'B':
					board.set_square(sq, BLACK_BISHOP);
					break;
				case 'Q':
					board.set_square(sq, BLACK_QUEEN);
					break;
				case 'K':
					board.set_square(sq, BLACK_KING);
					break;
				case 'P':
					board.set_square(sq, BLACK_PAWN);
					break;
				default:
					std::cerr << "Invalid fen string" << endl;
					clear_board();
					return;
				}
			}
		}
		_row++;
	}
	/* space */
	index++;
	/* side to move */
	if(fen[index++] == 'w') {
		board.side = WHITE;
		board.xside = BLACK;
	} else {
		board.side = BLACK;
		board.xside = WHITE;
	}
	/* space */
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
				castling_rights[BLACK_KING_SIDE] = true;
				break;
			case 'k':
				castling_rights[WHITE_KING_SIDE] = true;
				break;
			case 'q':
				castling_rights[WHITE_KING_SIDE] = true;
				break;
			default:
				std::cerr << "Invalid fen string" << endl;
				clear_board();
				return;
			}
		}
	}
	/* space */
	index++;
	/* enpassant square */
	if(fen[index] == '-') {
		set_enpassant(8);
	} else {
		char char_col = fen[index++];
		char char_row = fen[index++];
		int _col = char_col - 'a';
		set_enpassant(_col);
	}
	/* space */
	index++;
	std::string tmp_str = "";
	/* fifty move counter */
	while(fen[index] != ' ') {
		tmp_str += fen[index++];
	}
	try {
		fifty_move_ply = stoi(tmp_str);
	} catch(char* exception_name) {
		std::cerr << "Invalid fen string" << endl;
		clear_board();
		return;
	}
	/* space */
	index++;
	tmp_str.clear();
	/* move counter */
	while(index < (int)fen.size() && fen[index] != ' ') {
		tmp_str += fen[index++];
	}
	try {
		move_count = stoi(tmp_str);
	} catch(char* exception_name) {
		std::cerr << "Invalid fen string" << endl;
		clear_board();
		return;
	}
}

/* the piece in this case ranges from 0 to 5, so we need the _side parameter */
void Board::set_square(int sq, int non_side_piece, bool _side) {
    bits[non_side_piece + (_side == BLACK ? 6 : 0)] |= mask_sq(sq);
}

/* the piece in this case ranges from 0 to 11 */
void Board::set_square(int sq, int piece) {
    bits[piece] |= mask_sq(sq);
}

void Board::set_enpassant(int col) {
	enpassant = uint8_t(col);
}

/* the piece ranges from 0 to 5 */
void Board::clear_square(int sq, int non_side_piece, bool _side) {
    // assert(bits[non_side_piece + (_side == BLACK ? 6 : 0)] & mask_sq(sq));
    bits[non_side_piece] ^= mask_sq(sq);
}

/* the piece ranges from 0 to 11 */
void Board::clear_square(int sq, int piece) {
    // assert(bits[piece] & mask_sq(sq));
    bits[piece] ^= mask_sq(sq);
}

bool Board::is_empty(int sq) {
	const uint64_t mask = mask_sq(sq);
	for(int piece == WHITE_PAWN; piece <= BLACK_KING; piece++) {
		if(mask & bits[piece])
			return false;
	}
	return true;
}

uint64_t Board::get_all_mask() {
	uint64_t mask = 0;
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
		mask |= bits[piece];
	return mask;
}

/* returns the mask of all white/black pieces */
uint64_t Board::get_side_mask(bool side) {
	uint64_t mask = 0;
	if(side == WHITE) {
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

uint64_t Board::get_pawn_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_PAWN];
	return bits[BLACK_PAWN];
}

uint64_t Board::get_knight_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_KNIGHT];
	return bits[BLACK_KNIGHT];
}

uint64_t Board::get_bishop_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_BISHOP];
	return bits[BLACK_BISHOP];
}

uint64_t Board::get_rook_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_ROOK];
	return bits[BLACK_ROOK];
}

uint64_t Board::get_queen_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_QUEEN];
	return bits[BLACK_QUEEN];
}

uint64_t Board::get_king_mask(bool side) const {
	if(side == WHITE)
		return bits[WHITE_KING];
	return bits[BLACK_KING];
}

int Board::get_piece(int sq) const {
    uint64_t mask = mask_sq(sq);
    for(int piece = 0; piece < 12; piece++)
        if(mask & bits[i])
            return piece;
            // return ((piece >= 6) ? (piece - 6) : piece);
    return EMPTY;
}

/* returns whether a piece at square is WHITE, BLACK or EMPTY */
bool Board::get_color(int sq) const {
    uint64_t mask = mask_sq(sq);
    for(int piece = 0; piece < 12; piece++)
        if(mask & bits[i])
            return ((piece >= 6) ? BLACK : WHITE); 
    return EMPTY;
}

void Board::update_castling_rights(const Move& move) {
    int from  = move.get_from();
    int to    = move.get_to();
    int piece = get_piece(to);
    if(piece == WHITE_KING || piece == BLACK_KING) {
        if(side == WHITE) {
            castling_rights[WHITE_QUEEN_SIDE] = false;
            castling_rights[WHITE_KING_SIDE] = false;
        } else {
            castling_rights[BLACK_QUEEN_SIDE] = false;
            castling_rights[BLACK_KING_SIDE] = false;
        }
    } else if(piece == WHITE_ROOK || piece == BLACK_ROOK) {
        /* if the rook at that position is already off it doesn't change anything */
        switch(from) {
            case A1:
                castling_rights[WHITE_QUEEN_SIDE] = false;
                break;
            case H1:
                castling_rights[WHITE_KING_SIDE] = false;
                break;
            case A8:
                castling_rights[BLACK_QUEEN_SIDE] = false;
                break;
            case H8:
                castling_rights[BLACK_KING_SIDE] = false;
                break;
        }
    }
}

void Board::update_key(const Board& board_prev, Move move) {
    if(move.is_null_move())
        return;

    if(board_prev.enpassant != enpassant) {
        key ^= zobrist_enpassant[board_prev.enpassant];
        key ^= zobrist_enpassant[enpassant];
    }
    
    for(int castling_type = 0; castling_type < 4; castling_type++)
        if(board_prev.castling_rights[castling_type] != castling_rights[castling_type])
            key ^= zobrist_castling[castling_type];
        
    int from = move.get_from();
    int to   = move.get_to();
    int flag = move.get_flag();

    if(flag == QUIET_MOVE) {
        /* nothing to do */
    } else if(flag == CASTLING) {
        int rook_from = -1, rook_to = -1;
        if(from == E1 && to == C1) { rook_from = A1; rook_to = D1; } 
        else if(from == E1 && to == G1) { rook_from = H1; rook_to = E1; } 
        else if(from == E8 && to == C8) { rook_from = A8; rook_to = D1; }
        else if(from == E8 && to == G8) { rook_from = H8; rook_to = E8; }
        key ^= zobrist_pieces[ROOK + (side == BLACK ? 6 : 0)][rook_from];
        key ^= zobrist_pieces[ROOK + (side == BLACK ? 6 : 0)][rook_to];
    } else if(flag == ENPASSANT) {
        int adjacent = row(from) * 8 + col(to);
        key ^= zobrist_pieces[PAWN + (side == WHITE ? 6 : 0)][adjacent];
    } else if(flag == CAPTURE) {
        int piece_before = board_prev.get_piece(to);
        key ^= zobrist_pieces[piece_before][to];
    } else  {
        /* promotion */ 
        int promotion_piece = flag - 3; /* this is dangerous */
        key ^= zobrist_piece[PAWN + (side == BLACK ? 6 : 0)][from];    
        key ^= zobrist_piece[promotion_piece + (side == BLACK ? 6 : 0)][to];
    }       

    int piece = get_piece(to);
    key ^= zobrist_piece[piece][from]; 
    key ^= zobrist_piece[piece][to];

    key ^= zobrist_side[side];
    key ^= zobrist_side[xside];
}

bool Board::is_attacked(int sq) const {
    if(knight_attacks[sq] & get_knight_mask(xside))
        return true;

    if(king_attacks[sq] & get_king_mask(xside))
        return true;

    if(pawn_attacks[xside][sq] & get_pawn_mask(xside))
        return true;

    uint64_t occupation = get_all_mask();

    const uint64_t bishop_moves = Bmagic(sq, occupation);
    if((bishop_moves & get_bishop_mask(!side)) || (bishop_moves & get_queen_mask(!side)))
        return true;

    const uint64_t rook_moves = Rmagic(sq, occupation);
    if((rook_moves & get_rook_mask(!side)) || (rook_moves & get_queen_mask(!side)))
        return true;

    return false;
}

bool Board::in_check() const {
	int king_mask, king_pos;
	if(side == WHITE) {
		king_mask = board_history.back().bits[WHITE_KING];
	} else {
		king_mask = board_history.back().bits[BLACK_KING];
	}
	king_pos = LSB(king_mask);
	return is_attacked(king_pos);
}

bool Board::castling_valid(const Move& move) {
    const int from = move.get_from();
    const int to   = move.get_to();

    if(is_attacked(from))
        return false;

    bool white_queen_side = (side == WHITE) && (from == E1 && to == C1) && castling_rights[WHITE_QUEEN_SIDE];
    bool white_king_side  = (side == WHITE) && (from == E1 && to == G1) && castling_rights[WHITE_KING_SIDE];
    bool black_queen_side = (side == BLACK) && (from == E8 && to == C8) && castling_rights[BLACK_QUEEN_SIDE];
    bool black_king_side  = (side == BLACK) && (from == E8 && to == G8) && castling_rights[BLACK_KING_SIDE];

    if(white_queen_side || black_queen_side) {
        /* to the left */
        from--;
        if (is_attacked(from))
            return false;
        from++;
    } else if(white_king_side || black_king_side) {
        /* to the right */
        from++;
        if(is_attacked(from))
            return false;
        from--;
    } else {
        /* no bool has been activated */
        return false;
    }

    if(is_attacked(to))
        return false;

    return true;
}

/* it only works for pawns */
bool Board::move_diagonal(const Move& move) {
	int from = move.get_from();
	int to = move.get_to();
	if(side == WHITE) {
		if(to != from + 7 && to != from + 9)
			return false;
		/* wrong offset */
		if(col(to) == 0 && to == from + 7)
			return false;
		/* wrong offset */
		if(col(to) == 7 && to == from + 9)
			return false;
	} else {
		if(to != from - 7 && to != from - 9)
			return false;
		/* wrong offset */
		if(col(to) == 0 && to == from - 9)
			return false;
		/* wrong offset */
		if(col(to) == 7 && to == from - 7)
			return false;
	}
}

bool Board::check_pawn_move(Move move) {
    if(flag == ENPASSANT) {
        if(!is_move_diagonal(move))
            return false;
        int adjacent = 8 * row(from_sq) + col(to_sq);
        /* the enpassant flag doesn't match with column of square to */
        if(enpassant != col(adjacent))
            return false;
        /* there is only one row we can move to if we are eating enpassant */
        if(side == WHITE && row(to) != 5)
            return false;
        /* there is only one row we can move to if we are eating enpassant */
        if(side == BLACK && row(to) != 2)
            return false;
        /* if this fails there is a bug regarding the enpassant flag */
        assert(get_piece(to) == WHITE_PAWN || get_piece(to) == BLACK_PAWN);
    } else if(flag == CAPTURE) {
        /* we have to eat diagonally */
        if(!is_move_diagonal(move))
            return false;
        /* there must be an enemy piece at to square */
        if(mask_sq(to) & other_side_mask)
            return false;
    } else if(flag == QUIET_MOVE) {
        /* it can only go one forward or two forward */
        if(side == WHITE && (to != from + 8 && to != from + 16))
            return false;
        /* it can only go one forward or two forward */
        if(side == BLACK && (to != from - 8 && to != from - 16))
            return false;
        /* square in front can't be occupied */
        int one_forward = mask_sq(from + (side == WHITE ? 8 : -8));
        if(one_forward & my_side_mask & other_side_mask) /* it should return 0 */
            return false;
        /* if it does two-forward, that square can't be occupied */
        if(to == from + 16 || to == from - 16) {
            int two_forward = mask_sq(from + (side == WHITE ? 16 : -16));
            if(two_forward & my_side_mask & other_side_mask) /* it should return 0 */
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
        if(side == WHITE && row(to) != 7)
            return false;
        /* to square must be at the last/first row */
        if(side == BLACK && row(to) != 0)
            return false;
        /* to square can't be occupied */
        if(mask_sq(to) & my_side_mask & other_mask)
            return false;
    }
    return true;
}


void Board::make_move(const Move& move) {
	int from  = move.get_from();
	int to    = move.get_to();
	int flag  = move.get_flag();
	int piece = get_piece(from);

	if(move.is_null_move()) {
		/* don't do anything */
	} else if(flag == QUIET_MOVE) {
		clear_square(from, piece);
		set_square(to, piece);
	} else if(flag == CAPTURE) {
		int captured_piece = get_piece(to);
		clear_square(to, captured_piece);
		clear_square(from, piece);
		set_square(to, piece);
	} else if(flag == CASTLING) {
		int rook_from, rook_to;
		if(from == E1 && to == C1) { rook_from = A1; rook_to = D1; }
		else if(from == E1 && to == G1) { rook_from = H1; rook_to = E1; }
		else if(from == E8 && to == C8) { rook_from = A8; rook_to = D1; }
		else if(from == E8 && to == G8) { rook_from = H8; rook_to = E8; }
		else assert(false);
		clear_square(from, KING, side);
		clear_square(rook_from, ROOK, side);
		set_square(to, KING, side);
		set_square(rook_to, ROOK, side);
	} else if(flag == ENPASSANT) {
		int adjacent = row(from) * 8 + col(to);
		clear_square(from, piece);
		clear_square(adjacent, PAWN, !side);
		set_square(to, piece);
	} else {
		int promotion_piece = flag - 3;
		clear_square(from, PAWN, side);
		set_square(to, promotion_piece, !side); /* we have to pass the side too */
	}

	if(piece == WHITE_PAWN || piece == BLACK_PAWN || flag == CAPTURE)
		fifty_move_ply = -1;

	if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(from - to) == 16) {
		set_enpassant(col(from));
	} else {
		/* no column has enpassant */
		set_enpassant(8);
	}

	fifty_move_ply++;
	if(side == BLACK)
		move_count++;
	side = !side;
	xside = !side;
}

bool Position::move_valid(Move move) {
	const int from  = move.get_from();
	const int to    = move.get_to();
	const int flag  = move.get_flag();

	/* out of range */
	if(from < 0 || from >= 64 || to < 0 || to >= 64)
		return false;

	int piece = get_piece(from);

	/* trivial conditions that must be met */
	if(from == to || piece == EMPTY || color[to] == side || get_color(from) != side)
		return false;

	if(flag == CASTLING) {
		/* this is the only place were we return true before the end of the function */
		if(check_castling(move))
			return true;
	} else if(piece == PAWN) {
		if (!check_pawn_move(move))
			return false;
	} else {
		/* standardise the piece, kind of */
		piece -= side == BLACK ? 6 : 0;
		switch(piece) {
		case KING:
			if (!(mask_sq(to) && king_moves[from]))
				return false;
			break;
		case KNIGHT:
			if (!(mask_sq(to) && knight_moves[from]))
				return false;
			break;
		case BISHOP:
			if (!(mask_sq(to) && Bmoves(from, get_all_mask())))
				return false;
		case ROOK:
			if(!(mask_sq(to) && Rmoves(from, get_all_mask())))
				return false;
		case QUEEN:
			uint64_t occupation = get_all_mask();
			if(!(mask_sq(to) & Bmoves(from, occupation)) && !(mask_sq(to) & Rmoves(from, occupation)))
				return false;
		}
	}

	/* simulating make_move function */
	int captured_piece;
	if(flag == ENPASSANT) {
		int adjacent = 8 * row(from) + col(to);
		clear_square(adjacent, PAWN, !side);
		set_square(adjacent, piece);
		clear_square(from, piece);
	} else {
		captured_piece = get_piece(to);
		if(captured_piece != EMPTY)
			clear_square(to, captured_piece);
		set_square(to, piece);
		clear_square(from, piece);
	}

	const bool in_check_after_move = in_check();

	/* reverse the modifications */
	if(flag == ENPASSANT) {
		const int adjacent = 8 * row(from) + col(to);
		set_square(adjacent, PAWN, !side);
		clear_square(adjacent, piece);
		set_square(adjacent, piece);
	} else {
		if(captured_piece != EMPTY)
			set_square(to, captured_piece);
		clear_square(to, piece);
		set_square(from, piece);
	}

	if(in_check_after_move)
		return false;

	return true;
}
