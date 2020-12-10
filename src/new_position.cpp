#include <iostream>
#include <string>

void Position::make_move(Move move) {
    board_history.push_back(static_cast<BitBoard>(board_history.back()));    
    BitBoard& board = board_history.back(); 

    int from = move.get_from();
    int to = move.get_to();
    int flags = move.get_flags();
    int piece = board.get_piece(from);
    bool side = board.side;

    if(move.is_null_move()) {
        /* no need to do anything */
    } else if(flags == QUIET_MOVE) {
        board.clear_square(from, piece, side);
        board.set_square(to, piece, side);
    } else if(flags == CAPTURE) {        
        int captured_piece = board.get_piece(to);
        board.clear_square(to, captured_piece, !side);
        board.clear_square(from, piece, side);
        board.set_square(to, piece, side);
    } else if(flags == CASTLING) {
        int rook_from = -1, rook_to = -1;
        if(from == E1 && to == C1) { rook_from = A1; rook_to = D1; } 
        else if(from == E1 && to == G1) { rook_from = H1; rook_to = E1; } 
        else if(from == E8 && to == C8) { rook_from = A8; rook_to = D1; }
        else if(from == E8 && to == G8) { rook_from = H8; rook_to = E8; }
        else assert(castling_type != -1); 
        board.clear_square(from, KING, side);
        board.clear_square(rook_from, ROOK, side);
        board.set_square(to, KING, side);
        board.set_square(rook_to, ROOK, side);
    } else if(flags == ENPASSANT) {
        int adjacent = row(from) * 8 + col(to);
        board.clear_square(from, piece, side); 
        board.clear_square(adjacent, PAWN, !side);
        board.set_square(to, piece, side);
    } else {
        int promotion_piece = flags - 3; /* ! */
        board.clear_square(from, PAWN, side);
        board.set_square(to, promotion_piece, side);
    }

    if(piece == WHITE_PAWN || piece == BLACK_PAWN || flags == CAPTURE)
        board.reset_fifty_move_counter();

    if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(from - to) == 16)
        board.set_enpassant(col(from));
    else
        board.set_enpassant(8); /* this equals no enpassant */

    board.update_key(board_history[board_history.size() - 2], move);
    board.update_castling_rights(move);
    board.increment_counter();
    board.side = !board.side;
}

/* returns false if move is invalid, otherwise it applies the move and returns true */
bool Position::make_move(std::string str_move) {
    if(str_move.size() != 4
    || (str_move[0] < 'a' || str_move[0] > 'h')
    || (str_move[1] < '1' || str_move[1] > '8')
    || (str_move[2] < 'a' || str_move[2] > 'h')
    || (str_move[3] < 'a' || str_move[3] > '8')
        return false;

    BitBoard& board = board_history.back(); 
    int from = (str_move[1] - '1') * 8 + (str_move[0] - 'a');
    int to = (str_move[3] - '1') * 8 + (str_move[2] - 'a');
    int flags = 0;
    int piece = board.get_piece(from);
    bool side = board.side;

    if(piece == EMPTY)
        return false;

    if(piece == PAWN && (row(to) == 0 || row(to) == 7)) {
        if((side == WHITE && row(to) == 0) || (side == BLACK && row(to) == 7))
            return false;
        std::cout << "Which piece should the pawn be promoted to (q, r, b, k)? " << endl;
        std::string piece_str;
        std::cin >> piece_str;
        switch(piece[0]) {
            case 'q':
                flags = QUEEN_PROMOTION;
                break;
            case 'r':
                flags = ROOK_PROMOTION;
                break;
            case 'b':
                flags = BISHOP_PROMOTION;
                break;
            case 'k':
                flags = KNIGHT_PROMOTION;
                break;
        }
    } else if(piece == KING && abs(col(from) - col(to)) == 2) {
        flags = CASTLING;
    } else if(piece == PAWN && abs(col(from) - col(to)) == 1 && board.get_piece(to) == EMPTY) {
        flags = ENPASSANT;
    } else if(board.get_piece(to) != EMPTY) {
        flags = CAPTURE;
    } else {
        flags = QUIET_MOVE;
    }

    Move move = Move(from, to, flags);

    if(!move_is_valid(move))
        return false;

    board.make_move(move);
    return true;
}

/* take back the last move in move history */
void Position::take_back() {
    assert(!board_history.empty());
    board_history.pop_back();         
}

bool Position::move_is_valid(Move move) {
    /* ? */
}

bool Position::is_attacked(int sq) {
    /* ? */
}

bool Position::in_check(bool side) {
    int king_mask, king_pos;
    if(side == WHITE) {
        king_mask = board_history.back().bits[WHITE_KING];
    } else {
        king_mask = board_history.back().bits[BLACK_KING];
    }
    king_pos = msb(king_mask); 
    return is_attacked(king_pos);
}

void Position::set_from_fen() {
    /* ? */     
}


