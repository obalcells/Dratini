
/* accessing board parameters */
bool BitBoard::is_empty(int sq) {
    return (sq & get_white() & get_black()) == 0;
}

int BitBoard::get_piece(int sq) {
    for(int piece = 0; piece < 12; piece++)
        if(sq & bits[i])
            return ((piece >= 6) ? (piece - 6) : piece);
    return EMPTY;
}

bool BitBoard::get_color(int sq) {
    for(int piece = 0; piece < 12; piece++)
        if(sq & bits[i])
            return ((piece >= 6) ? BLACK : WHITE); 
    return EMPTY;
}

uint64_t BitBoard::get_all() {
    uint64_t mask = 0;
    for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++)
        mask |= bits[piece];
    return mask;
}

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
    else
        return bits[BLACK_PAWN];
}

uint64_t BitBoard::get_knights(bool side) const {
    if(side == WHITE)
        return bits[WHITE_KNIGHT];
    else
        return bits[BLACK_KNIGHT];
}

uint64_t BitBoard::get_bishops(bool side) const {
    if(side == WHITE)
        return bits[WHITE_BISHOP];
    else
        return bits[BLACK_BISHOP];
}

uint64_t BitBoard::get_rooks(bool side) const {
    if(side == WHITE)
        return bits[WHITE_ROOK];
    else
        return bits[BLACK_ROOK];
}

uint64_t BitBoard::get_queens(bool side) const {
    if(side == WHITE)
        return bits[WHITE_QUEEN];
    else
        return bits[BLACK_QUEEN];
}

uint64_t BitBoard::get_king(bool side) const {
    if(side == WHITE)
        return bits[WHITE_KING];
    else
        return bits[BLACK_KING];
}

/* changing board parameters */
void BitBoard::set_enpassant(int col) {
    enpassant = col;
}

void update_castling_rights(Move move) {
    int from  = move.get_from();
    int to    = move.get_to();
    int piece = get_piece(to);
    if(piece == KING) {
        if(side == WHITE) {
            castling_rights[WHITE_QUEEN_SIDE] = false;
            castling_rights[WHITE_KING_SIDE] = false;
        } else {
            castling_rights[BLACK_QUEEN_SIDE] = false;
            castling_rights[BLACK_KING_SIDE] = false;
        }
    } else if(piece == ROOK) {
        /* if the rook at that position is already off it does'nt change anything */
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

void increment_count() {
    fifty_move_count++;    
    move_count++;
}

void reset_fifty_move_count() {
    fifty_move_count = -1;
}

void increment_zobrist() {
    /* ? */
}

