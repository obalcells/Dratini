#include <iostream>
#include <string>

void Position::set_from_fen(const std::string& fen) {
	board_history.clear();
	board_history.push_back(BitBoard(fen));
	/* reset the other variables, which we don't have yet */
}

Board Position::get_board() {
	return board_history.back();
}

uint64_t Position::get_key() {
	return board_history.back().key();
}

int Position::get_move_count() {
	return board_history.back().move_count;
}

void Position::make_move(const Move& move) {
    board_history.push_back(static_cast<BitBoard>(board_history.back()));
    Board& board = board_history.back();
    board.make_move(move);
    board.update_key(board_history[board_history.size() - 2], move);
}

/* returns false if move is invalid, otherwise it applies the move and returns true */
bool Position::make_move(const std::string& str_move) {
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

    if(!board.move_valid(move))
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
