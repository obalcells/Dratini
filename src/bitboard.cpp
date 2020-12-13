
static uint64_t mask_sq(int sq) {
    return (uint64_t(1) << sq);
}

/* the piece in this case ranges from 0 to 5, so we need the _side parameter */
void set_square(int sq, int non_side_piece, bool _side) {
    bits[non_side_piece + (_side == BLACK ? 6 : 0)] |= mask_sq(sq);
}

/* the piece in this case ranges from 0 to 11 */
void set_square(int sq, int piece) {
    bits[piece] |= mask_sq(sq);
}

/* the piece ranges from 0 to 5 */
void clear_square(int sq, int non_side_piece, bool _side) {
    assert(bits[non_side_piece + (_side == BLACK ? 6 : 0)] & mask_sq(sq));
    bits[non_side_piece] ^= mask_sq(sq);
}

/* the piece ranges from 0 to 12 */
void clear_square(int sq, int piece) {
    assert(bits[piece] & mask_sq(sq));
    bits[piece] ^= mask_sq(sq);
}

bool BitBoard::is_empty(int sq) {
    return (mask_sq(sq) & get_white() & get_black()) == 0;
}

int BitBoard::get_piece(int sq) {
    uint64_t mask = mask_sq(sq);
    for(int piece = 0; piece < 12; piece++)
        if(mask & bits[i])
            return piece;
            // return ((piece >= 6) ? (piece - 6) : piece);
    return EMPTY;
}

/* returns whether a piece at square is WHITE, BLACK or EMPTY */
bool BitBoard::get_color(int sq) {
    uint64_t mask = mask_sq(sq);
    for(int piece = 0; piece < 12; piece++)
        if(mask & bits[i])
            return ((piece >= 6) ? BLACK : WHITE); 
    return EMPTY;
}

uint64_t BitBoard::get_all() {
    uint64_t mask = 0;
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        mask |= bits[piece];
    return mask;
}

/* returns the mask of all white/black pieces */
uint64_t BitBoard::get_side(bool side) {
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

uint64_t BitBoard::get_pawns(bool side) const {
    if(side == WHITE)
        return bits[WHITE_PAWN];
    return bits[BLACK_PAWN];
}

uint64_t BitBoard::get_knights(bool side) const {
    if(side == WHITE)
        return bits[WHITE_KNIGHT];
    return bits[BLACK_KNIGHT];
}

uint64_t BitBoard::get_bishops(bool side) const {
    if(side == WHITE)
        return bits[WHITE_BISHOP];
    return bits[BLACK_BISHOP];
}

uint64_t BitBoard::get_rooks(bool side) const {
    if(side == WHITE)
        return bits[WHITE_ROOK];
    return bits[BLACK_ROOK];
}

uint64_t BitBoard::get_queens(bool side) const {
    if(side == WHITE)
        return bits[WHITE_QUEEN];
    return bits[BLACK_QUEEN];
}

uint64_t BitBoard::get_king(bool side) const {
    if(side == WHITE)
        return bits[WHITE_KING];
    return bits[BLACK_KING];
}

void BitBoard::set_enpassant(int col) {
    enpassant = col;
}

void BitBoard::update_castling_rights(Move move) {
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

void BitBoard::increment_count() {
    fifty_move_count++;    
    move_count++;
}

void BitBoard::reset_fifty_move_count() {
    fifty_move_count = -1;
}

void BitBoard::update_key(const BitBoard& board_prev, Move move) {
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
}

bool BitBoard::in_check() const {
    int king_mask, king_pos;
    if(side == WHITE) {
        king_mask = board_history.back().bits[WHITE_KING];
    } else {
        king_mask = board_history.back().bits[BLACK_KING];
    }
    king_pos = most_significant_bit(king_mask);
    return is_attacked(king_pos);
} 

bool BitBoard::is_attacked(int sq) {
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

bool BitBoard::castling_valid(Move move) {
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
        if (is_attacked(from - 1))
            return false;
    } else if(white_king_side || black_king_side) {
        /* to the right */
        if(is_attacked(from + 1))
            return false;
    } else {
        /* no bool has been activated */
        return false;
    }

    if(is_attacked(to))
        return false;

    return true;
}

bool BitBoard::check_pawn_move(Move move) {
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

/* it only works for pawns */
bool BitBoard::move_diagonal(const Move& move) {
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
