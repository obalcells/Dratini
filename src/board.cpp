#include <vector>
#include <iostream>

#include "magicmoves.h"
#include "board.h"
#include "defs.h"
#include "gen.h"

static const char piece_char[12] = { 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k' };
static std::string str_seed = "Dratini is fast!";
static std::seed_seq seed(str_seed.begin(), str_seed.end());
static std::mt19937_64 rng(seed);
static std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
static bool required_data_initialized = false; // this must be set to false at the beginning
static std::vector<std::vector<uint64_t> > zobrist_pieces;
static std::vector<uint64_t> zobrist_castling;
static std::vector<uint64_t> zobrist_enpassant;
static std::vector<uint64_t> zobrist_side;

std::vector<std::vector<uint64_t> > pawn_attacks;
std::vector<uint64_t> knight_attacks;
std::vector<uint64_t> king_attacks;
std::vector<uint64_t> castling_mask;

static uint64_t get_random_64() {
	return dist(rng);
}

/* initializes zobrist hashes and bitboard tables */
static void init_data() {
    if(required_data_initialized) {
        return;
	}

	cerr << "Initalising data..." << endl;

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

	cerr << "Finished initializing data" << endl;

    required_data_initialized = true;

	cerr << "Finished initializing data" << endl;
}

Board::Board(const std::string& str) { // bool read_from_file = false) {
	cerr << "Before initializing data" << endl;
	init_data();
	cerr << "Finished doing the things with the data" << endl;
	// if(read_from_file == false) {
	// 	set_from_fen(str);
	// } else {
	// 	set_from_file(str);
	// }
	set_from_fen(str);
	// set_from_data();
	update_classic_board();
	key = calculate_key();
}

Board::Board() {
	init_data();
	cerr << "After initializing data..." << endl;
	// set_from_data();
	set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	cerr << "After setting from fen..." << endl;
	update_classic_board();
	key = calculate_key();
	cerr << "After updating classical board" << endl;
}

void Board::clear_board() {
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
		bits[piece] &= 0;
}

void Board::set_from_fen(const std::string& fen) {
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
	cerr << "8" << endl;
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
	cerr << "9" << endl;
	try {
		fifty_move_ply = stoi(tmp_str);
	} catch(char* exception_name) {
		std::cout << "Invalid fen string (10)" << endl;
		clear_board();
		return;
	}
	cerr << "10" << endl;
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
	// key = calculate_key();
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
    bits[non_side_piece + (_side == BLACK ? 6 : 0)] ^= mask_sq(sq);
}

/* the piece ranges from 0 to 11 */
void Board::clear_square(int sq, int piece) {
    // assert(bits[piece] & mask_sq(sq));
    bits[piece] ^= mask_sq(sq);
}

bool Board::is_empty(int sq) {
	const uint64_t mask = mask_sq(sq);
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
		if(mask & bits[piece])
			return false;
	}
	return true;
}

uint64_t Board::get_all_mask() const {
	uint64_t mask = 0;
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
		mask |= bits[piece];
	return mask;
}

uint64_t Board::get_piece_mask(int piece) const {
	return bits[piece];
}

/* returns the mask of all white/black pieces */
uint64_t Board::get_side_mask(bool _side) const {
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

uint64_t Board::get_pawn_mask(bool _side) const {
	if(_side == WHITE)
		return bits[WHITE_PAWN];
	return bits[BLACK_PAWN];
}

uint64_t Board::get_knight_mask(bool _side) const {
	if(_side == WHITE)
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
	// if(color_at[sq] == EMPTY) {
	// 	return EMPTY;
	// }
	// return color_at[sq] == WHITE ? piece_at[sq] : piece_at[sq] + 6;

    uint64_t mask = mask_sq(sq);
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
        if(mask & bits[piece]) {
            return piece;
		}
	}
    return EMPTY;
}

/* returns whether a piece at square is WHITE, BLACK or EMPTY */
int Board::get_color(int sq) const {
    uint64_t mask = mask_sq(sq);
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        if(mask & bits[piece])
            return ((piece >= BLACK_PAWN) ? BLACK : WHITE); 
    return EMPTY;
}

void Board::update_castling_rights(const Move move) {
    const int piece = get_piece(get_to(move));

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

uint64_t Board::calculate_key() const {
	uint64_t new_key = 0;

	new_key ^= zobrist_enpassant[enpassant];

    for(int castling_type = 0; castling_type < 4; castling_type++) {
		if(castling_rights[castling_type]) {
			new_key ^= zobrist_castling[castling_type];
		}
	}

	int piece;
	for(int sq = 0; sq < 64; sq++) {
		piece = get_piece(sq);
		if(piece != EMPTY) {
			new_key ^= zobrist_pieces[piece][sq];   
		}
	}

	new_key ^= zobrist_side[side];

	return new_key;
}

void Board::update_key(const Board& board_before, const Move move) {
    if(is_null(move)) {
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
        
    const int from_sq = get_from(move);
    const int to_sq = get_to(move);
    const int flag = get_flag(move);
	const int piece = board_before.get_piece(from_sq);

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
		if(captured_piece != EMPTY) {
			key ^= zobrist_pieces[captured_piece][to_sq];
		}
        key ^= zobrist_pieces[(side == WHITE ? BLACK_PAWN : WHITE_PAWN)][from_sq];
        key ^= zobrist_pieces[promotion_piece + (side == BLACK ? 0 : 6)][to_sq];
    }       
	key ^= zobrist_side[side];
    key ^= zobrist_side[xside];
}

bool Board::is_attacked(const int sq) const {
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

bool Board::checkmate() {
	if(!in_check()) {
		return false;
	}
	std::vector<Move> possible_moves;
	generate_moves(possible_moves, this);
	return possible_moves.empty();
}

bool Board::stalemate() {
	if(in_check()) {
		return false;
	}
	std::vector<Move> possible_moves;
	generate_moves(possible_moves, this);
	return possible_moves.empty();
}

bool Board::is_draw() const {
	if(fifty_move_ply >= 100) {
		return true;
	}
     
    // Insufficient material:
    //    (no pawns && no rooks && no queens)
    // && (at least one side with only one piece)
    // && (   (no more than 1 bishop or knight in both sides)
    //     || (no bishops and no more than 2 knights))
    
	if(
	   !(get_piece_mask(WHITE_PAWN) | get_piece_mask(BLACK_PAWN) | get_piece_mask(WHITE_ROOK) | get_piece_mask(BLACK_ROOK) | get_piece_mask(WHITE_QUEEN) | get_piece_mask(BLACK_QUEEN))
	&& (!(get_side_mask(WHITE) ^ get_king_mask(WHITE)) || !(get_side_mask(BLACK) ^ get_king_mask(BLACK)))
    && (   (popcnt(get_side_mask(WHITE) | get_side_mask(BLACK)) <= 2)
        || (!(get_bishop_mask(WHITE) | get_bishop_mask(BLACK)) && popcnt(get_knight_mask(WHITE) | get_knight_mask(BLACK)) <= 2))
	) {
		return true;
	}	

    // draw by repetition
    // later...
   
    return false;
}

bool Board::in_check() const {
	if(side == WHITE) {
		return is_attacked(lsb(bits[WHITE_KING]));
	} else {
		return is_attacked(lsb(bits[BLACK_KING]));
	}
}

bool Board::castling_valid(const Move move) const {
    int from_sq = get_from(move);
    int to_sq   = get_to(move);

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
bool Board::move_diagonal(const Move move) const {
	int from_sq = get_from(move);
	int to_sq   = get_to(move);
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

bool Board::check_pawn_move(const Move move) const {
    int from_sq = get_from(move); 
    int to_sq   = get_to(move);
    int flag    = get_flag(move);

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
		int new_flag = (get_piece(to_sq) == EMPTY ? QUIET_MOVE : CAPTURE_MOVE);
		Move move_with_changed_flag = Move(from_sq, to_sq, new_flag); 
		if(!check_pawn_move(move_with_changed_flag))
			return false;
    }
    return true;
}


void Board::make_move(const Move move) {
	int from_sq  = get_from(move);
	int to_sq    = get_to(move);
	int flag     = get_flag(move);
	int piece    = get_piece(from_sq);

	if(is_null(move)) {
		piece = -1;
		flag  = -1;
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
		if(get_piece(to_sq) != EMPTY)
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
	if(side == BLACK) {
		move_count++;
	}
	side = !side;
	xside = !side;

	update_classic_board();
}

bool Board::fast_move_valid(const Move move) const {
	const int piece_from = get_piece(get_from(move));
	const int piece_to = get_piece(get_to(move));
	const int from_sq = get_from(move);
	const int to_sq = get_to(move);

	/*
	clear_square(from_sq, piece_from);
	if(piece_to != EMPTY)
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
	// quick_check("Move valid beg");

	int from_sq = get_from(move);
	int to_sq   = get_to(move);
	int flag    = get_flag(move);

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
	if(from_sq == to_sq || piece == EMPTY || get_color(to_sq) == side || get_color(from_sq) != side)
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
		assert(get_piece(to_sq) == EMPTY);
		set_square(to_sq, piece);
		// quick_check("Enpassant 2");
		assert(get_piece(from_sq) == WHITE_PAWN || get_piece(from_sq) == BLACK_PAWN);
		clear_square(from_sq, piece);
		assert(adjacent != to_sq && adjacent != from_sq);
		// quick_check("Enpassant");
	} else {
		captured_piece = get_piece(to_sq);
		if(captured_piece != EMPTY)
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
		if(captured_piece != EMPTY)
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

void Board::print_board() const {
	std::cout << endl;
	int i;
	for(i = 56; i >= 0;) {
		if(i % 8 == 0) std::cout << (i / 8) + 1 << "  ";
		std::cout << " ";
		if(get_piece(i) == EMPTY) {
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
	if(enpassant == 8) {
		std::cout << "Enpassant: None" << endl;
	} else {
		std::cout << "Enpassant: " << int(enpassant) << endl;
	}
	// cout << "Key is:" << endl; 
	// cout << key << endl;
	std::cout << "Castling (WQ, WK, BQ, BK): ";
	for(i = 0; i < 4; i++)
		std::cout << (castling_rights[i] ? "Y " : "N ");
	std::cout << endl;
	std::cout << "Side: " << (side == WHITE ? "WHITE" : "BLACK") << endl;
	// std::cout << "xSide: " << (xside == WHITE ? "WHITE" : "BLACK") << endl;
}

void Board::print_bitboard(uint64_t bb) const {
	std::cout << endl;
	int i;
	for(i = 56; i >= 0;) {
		if(i % 8 == 0) std::cout << (i / 8) + 1 << "  ";
		std::cout << " ";
		if(bb & (uint64_t(1) << i)) {
			if(get_piece(i) == EMPTY) {
				std::cout << 'x';
			} else {
				std::cout << piece_char[get_piece(i)];
			}
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
	for(i = 0; i < 8; i++) {
		std::cout << " " << char('a' + i);
	}
	std::cout << endl;
}

void Board::update_classic_board() {
	int piece;
	for(int sq = 0; sq < 64; sq++) {
		piece = get_piece(sq);
		if(piece == EMPTY) {
			piece_at[sq] = color_at[sq] = EMPTY;
		} else {
			piece_at[sq] = piece >= BLACK_PAWN ? piece - 6 : piece; 
			color_at[sq] = piece >= BLACK_PAWN ? BLACK : WHITE;
		}
	}
}

void Board::quick_check(const std::string& error_message) {
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

bool Board::same(const Board& other) const {
	if(other.castling_rights[WHITE_QUEEN_SIDE] != castling_rights[WHITE_QUEEN_SIDE])
		return false;
	if(other.castling_rights[WHITE_KING_SIDE] != castling_rights[WHITE_KING_SIDE])
		return false;
	if(other.castling_rights[BLACK_QUEEN_SIDE] != castling_rights[BLACK_QUEEN_SIDE])
		return false;
	if(other.castling_rights[BLACK_KING_SIDE] != castling_rights[BLACK_KING_SIDE])
		return false;

	for(int i = 0; i < 64; i++) {
		if(other.get_piece(i) != get_piece(i)) {
			return false;
		}
	}

	return true;
}

// void Board::same(const Position& other) const {
// 	if((bool(other.castling & 1) && bool(other.castling & 4)) != castling_rights[WHITE_QUEEN_SIDE])
// 		throw("Castling is different");
// 	if((bool(other.castling & 2) && bool(other.castling & 4)) != castling_rights[WHITE_KING_SIDE])
// 		throw("Castling is different");
// 	if((bool(other.castling & 8) && bool(other.castling & 32)) != castling_rights[BLACK_QUEEN_SIDE])
// 		throw("Castling is different");
// 	if((bool(other.castling & 16) && bool(other.castling & 32)) != castling_rights[BLACK_KING_SIDE])
// 		throw("Castling is different");

// 	for(int i = 0; i < 64; i++) {
// 		if(other.piece[i] + (other.color[i] == BLACK ? 6 : 0) != get_piece(i)) {
// 			if(other.piece[i] == EMPTY) { 
// 				if(get_piece(i) != EMPTY) {
// 					throw("Boards are different");				
// 				}
// 			} else {
// 				if(other.piece[i] + (other.color[i] == BLACK ? 6 : 0) != get_piece(i)) {
// 					std::cerr << "Diff at " << pos_to_str(i) << " " << other.piece[i] + (other.color[i] == BLACK ? 6 : 0) << " " << get_piece(i) << endl;
// 					throw("Boards are different");
// 				}
// 			}
// 		}
// 	}
// }

void Board::print_bitboard_data() const {
	cout << endl << endl;
	cout << '"';
	for(int pos = 0; pos < 64; pos++) {
		if(pos > 0) cout << ' ';
		cout << (int)get_piece(pos);
	}
	cout << ' ' << key;
	cout << ' ' << move_count;
	cout << ' ' << (int)fifty_move_ply;
	cout << ' ' << (int)enpassant;
	cout << ' ' << castling_rights[0]; 
	cout << ' ' << castling_rights[1]; 
	cout << ' ' << castling_rights[2]; 
	cout << ' ' << castling_rights[3]; 
	cout << ' ' << side;
	cout << ' ' << xside;
	cout << '"' << endl << endl;
}

void Board::set_from_data() {
	// freopen(file_name.c_str(), "r", stdin);	
	// std::string data = "3 1 2 4 12 2 1 3 0 12 0 12 5 0 0 0 12 12 12 12 12 12 12 12 12 0 12 0 0 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 6 7 12 12 6 6 6 6 12 6 6 6 9 7 8 10 11 12 12 9 1106155485056437330 4 2 8 0 0 1 1 1 0";
	std::string data = "3 1 2 4 12 2 1 3 0 12 0 12 5 0 0 0 12 12 12 12 12 12 12 12 12 0 12 0 0 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 6 7 12 12 6 6 6 6 12 6 6 6 9 7 8 10 11 12 12 9 1106155485056437330 4 2 8 0 0 1 1 1 0";
	int piece;
	for(int i = 0; i <= BLACK_KING; i++) {
		bits[i] = 0;
	}
	int i = 0;
	for(int pos = 0; pos < 64; pos++) {
		std::string this_number = "";
		while(i < (int)data.size() && data[i] != ' ') {
			this_number += data[i++];
		}
		i++;
		piece = stoi(this_number);
		bits[piece] |= mask_sq(pos);		
		cerr << piece << " ";
	}
	side = BLACK;
	xside = WHITE;
	// cerr << endl;
	// key = 0;
	// cin >> key;
	// for the fucking chars
	// cin >> piece;
	// move_count = piece;
	// cin >> piece;
	// fifty_move_ply = piece;
	// cin >> piece;
	// enpassant = piece;
	enpassant = NO_ENPASSANT;
	// cin >> castling_rights[0]; 
	// cin >> castling_rights[1]; 
	// cin >> castling_rights[2]; 
	// cin >> castling_rights[3]; 
	// cin >> side;
	// cin >> xside;
}
