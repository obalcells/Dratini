
bool BitBoard::is_empty(int sq) {
    return (sq & get_white() & get_black()) == 0;
}

int BitBoard::get_piece(int sq) {
    for(int piece = 0; piece < 12; piece++)
        if(sq & bits[i])
            return piece;
            // return ((piece >= 6) ? (piece - 6) : piece);
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
    int to = move.get_to();
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
        int promotion_piece = flags - 3;
        key ^= zobrist_piece[PAWN + (side == BLACK ? 6 : 0)][from];    
        key ^= zobrist_piece[promotion_piece + (side == BLACK ? 6 : 0)][to];
    }       

    int piece = get_piece(to);
    key ^= zobrist_piece[piece][from]; 
    key ^= zobrist_piece[piece][to];
}

