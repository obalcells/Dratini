#include <iostream>
#include <string>

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
        if(!move_diagonal(move))
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
        uint64_t one_forward = mask_sq(from + (side == WHITE ? 8 : -8));
        if(one_forward & my_side_mask & other_side_mask) /* it should return 0 */
            return false;
        /* if it does two-forward, that square can't be occupied */
        if(to == from + 16 || to == from - 16) {
            uint64_t two_forward = mask_sq(from + (side == WHITE ? 16 : -16));
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

bool Position::move_valid(Move move) {
    BitBoard& board = board_history.back();
    const int from  = move.get_from();
    const int to    = move.get_to();
    const int flag  = move.get_flag();

    /* out of range */
    if(from < 0 || from >= 64 || to < 0 || to >= 64)
        return false;

    int piece = board.get_piece(from);

    /* trivial conditions that must be met */
    if(from == to || piece == EMPTY || color[to] == side || board.get_color(from) != side)
        return false;

    if(flag == CASTLING) {
        /* this is the only place were we return true before the end of the function */
        if(board.check_castling(move))
            return true;
    } else if(piece == PAWN) {
        if (!board.check_pawn_move(move))
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
                if (!(mask_sq(to) && Bmoves(from, board.get_all_mask())))
                    return false;
            case ROOK:
                if(!(mask_sq(to) && Rmoves(from, board.get_all_mask())))
                    return false;
            case QUEEN:
                uint64_t occupation = board.get_all_mask();
                if(!(mask_sq(to) & Bmoves(from, occupation)) && !(mask_sq(to) & Rmoves(from, occupation)))
                    return false;
        }
    }

    /* simulating make_move function */
    int captured_piece;
    if(flag == ENPASSANT) {
        int adjacent = 8 * row(from) + col(to);
        board.clear_square(adjacent, PAWN, !side);
        board.set_square(adjacent, piece);
        board.clear_square(from, piece);
    } else {
        captured_piece = board.get_piece(to);
        if(captured_piece != EMPTY)
            board.clear_square(to, captured_piece);
        board.set_square(to, piece);
        board.clear_square(from, piece);
    }

    bool in_check_after_move = board.in_check();

    /* reverse the board modifications */
    if(flag == ENPASSANT) {
        int adjacent = 8 * row(from) + col(to);
        board.set_square(adjacent, PAWN, !side);
        board.clear_square(adjacent, piece);
        board.set_square(adjacent, piece);
    } else {
        if(captured_piece != EMPTY)
            board.set_square(to, captured_piece);
        board.clear_square(to, piece);
        board.set_square(from, piece);
    }

    if(in_check_after_move)
        return false;

    return true;
}

void Position::make_move(Move move) {
    board_history.push_back(static_cast<BitBoard>(board_history.back()));    
    BitBoard& board = board_history.back();

    int from  = move.get_from();
    int to    = move.get_to();
    int flag  = move.get_flag();
    int piece = get_piece(from);

    if(move.is_null_move()) {
        /* don't do anything */
    } else if(flag == QUIET_MOVE) {
        board.clear_square(from, piece);
        board.set_square(to, piece);
    } else if(flag == CAPTURE) {        
        int captured_piece = board.get_piece(to);
        board.clear_square(to, captured_piece);
        board.clear_square(from, piece);
        board.set_square(to, piece);
    } else if(flag == CASTLING) {
        int rook_from, rook_to;
        if(from == E1 && to == C1) { rook_from = A1; rook_to = D1; } 
        else if(from == E1 && to == G1) { rook_from = H1; rook_to = E1; } 
        else if(from == E8 && to == C8) { rook_from = A8; rook_to = D1; }
        else if(from == E8 && to == G8) { rook_from = H8; rook_to = E8; }
        else assert(false); 
        board.clear_square(from, KING, side);
        board.clear_square(rook_from, ROOK, side);
        board.set_square(to, KING, side);
        board.set_square(rook_to, ROOK, side);
    } else if(flag == ENPASSANT) {
        int adjacent = row(from) * 8 + col(to);
        board.clear_square(from, piece);
        board.clear_square(adjacent, PAWN, !side);
        board.set_square(to, piece);
    } else {
        int promotion_piece = flag - 3;
        board.clear_square(from, PAWN, side);
        board.set_square(to, promotion_piece, !side); /* we have to pass the side too */
    }

    if(piece == WHITE_PAWN || piece == BLACK_PAWN || flag == CAPTURE)
        board.reset_fifty_move_counter();

    if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(from - to) == 16) {
        board.set_enpassant(col(from));
    } else {
        /* no column has enpassant */
        board.set_enpassant(8);
    }

    board.update_key(board_history[board_history.size() - 2], move);
    board.increment_count();
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

    if(piece == PAWN && (row(to) == 0 || row(to) == 7)) {
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
            default:
                std::cout << "Invalid piece" << endl;
                return false;
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

    if(!move_valid(move))
        return false;

    make_move(move);
    return true;
}

/* take back the last move in move history */
void Position::take_back() {
    assert(!board_history.empty());
    board_history.pop_back();         
}

bool Position::in_check() {
    return board_history.back().in_check();
}

void Position::set_from_fen(std::string fen) {
    board_history.clear();
    board_history.push_back(BitBoard(fen));
    /* reset the other variables */
}


