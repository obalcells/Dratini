/*
#include <iostream>
#include <string>
#include <cassert>

#include "position.h"
#include "board.h"
#include "defs.h"

Position::Position() {
    board_history.push_back(static_cast<Board>(Board()));
}

Position::Position(const std::string& str, bool read_from_file) {
    // we ignore read_from_file
    board_history.push_back(static_cast<Board>(Board(str)));
}

Position::~Position() {
    board_history.clear();
}

void Position::set_from_fen(const std::string& fen) {
	board_history.clear();
	board_history.push_back(static_cast<Board>(Board(fen)));
}

Board& Position::get_board() {
	return board_history.back();
}

uint64_t Position::get_key() const {
	return board_history.back().key;
}

int Position::get_move_count() const {
	return board_history.back().move_count;
}

void Position::make_move(const Move move) {
    board_history.push_back(static_cast<Board>(board_history.back()));
    move_history.push_back(static_cast<Move>(move));
    Board& board = board_history.back();
    assert(board.key == board.calculate_key());
    board.make_move(move);
    board.check_classic();
    board.key = board.calculate_key();
    // board.update_key(board_history[board_history.size() - 2], move);
    // assert(board.key == board.calculate_key());
    if(false && board.key != board.calculate_key()) {
        std::cerr << "Move is: " << move_to_str(get_from(move), get_to(move)) << endl;
        std::cerr << "Board was:" << endl;
        board_history[board_history.size() - 2].print_board();
        std::cerr << "Board now is" << endl;
        board.print_board();
        assert(board.key == board.calculate_key());
    }
}

// returns false if move is invalid, otherwise it applies the move and returns true
bool Board::make_move_from_str(const std::string& str_move) {
    cerr << "Making move from string" << endl;

    if(str_move.size() != 4
    || (str_move[0] < 'a' || str_move[0] > 'h')
    || (str_move[1] < '1' || str_move[1] > '8')
    || (str_move[2] < 'a' || str_move[2] > 'h')
    || (str_move[3] < '1' || str_move[3] > '8'))
        return false;

    Board& board = board_history.back(); 
    int from_sq = (str_move[1] - '1') * 8 + (str_move[0] - 'a');
    int to_sq = (str_move[3] - '1') * 8 + (str_move[2] - 'a');
    int flags = 0;
    int piece = board.get_piece(from_sq);
    bool side = board.side;

    if(piece == EMPTY) {
        return false;
    }

    if((piece == BLACK_PAWN || piece == WHITE_PAWN) && (row(to_sq) == 0 || row(to_sq) == 7)) {
        if((side == WHITE && row(to_sq) == 0) || (side == BLACK && row(to_sq) == 7))
            return false;
        flags = QUEEN_PROMOTION;
        // std::cout << "Which piece should the pawn be promoted to (q, r, b, k)? " << endl;
        // std::string piece_str;
        // std::cin >> piece_str;
        // switch(piece) {
        //     case 'q':
        //         flags = QUEEN_PROMOTION;
        //         break;
        //     case 'r':
        //         flags = ROOK_PROMOTION;
        //         break;
        //     case 'b':
        //         flags = BISHOP_PROMOTION;
        //         break;
        //     case 'k':
        //         flags = KNIGHT_PROMOTION;
        //         break;
        //     default:
        //         std::cout << "Invalid piece" << endl;
        //         return false;
        // }
    } else if((piece == WHITE_KING || piece == BLACK_KING) && abs(col(from_sq) - col(to_sq)) == 2) {
        flags = CASTLING_MOVE;
    } else if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(col(from_sq) - col(to_sq)) == 1 && board.get_piece(to_sq) == EMPTY) {
        flags = ENPASSANT_MOVE;
    } else if(board.get_piece(to_sq) != EMPTY) {
        flags = CAPTURE_MOVE;
    } else {
        flags = QUIET_MOVE;
    }

    Move move = Move(from_sq, to_sq, flags);
        
    cerr << "Already have move" << endl;

    if(!board.move_valid(move))
        return false;

    make_move(move);

    return true;
}

void Position::print_board() const {
    board_history.back().print_board();
}

bool Position::move_valid(const Move move) {
    return board_history.back().move_valid(move);
}

// returns false if move is invalid, otherwise it applies the move and returns true
bool Position::move_valid(int from_sq, int to_sq) {
    Board& board = board_history.back(); 
    int flags = 0;
    int piece = board.get_piece(from_sq);
    bool side = board.side;

    if(piece == EMPTY)
        return false;

    if((piece == WHITE_PAWN || piece == BLACK_PAWN) && (row(to_sq) == 0 || row(to_sq) == 7)) {
        if((side == WHITE && row(to_sq) == 0) || (side == BLACK && row(to_sq) == 7)) {
            return false;
        }
        flags = QUEEN_PROMOTION;
    } else if((piece == WHITE_KING || piece == BLACK_KING) && abs(col(from_sq) - col(to_sq)) == 2) {
        flags = CASTLING_MOVE;
    } else if((piece == WHITE_PAWN || piece == BLACK_PAWN) && abs(col(from_sq) - col(to_sq)) == 1 && board.get_piece(to_sq) == EMPTY) {
        flags = ENPASSANT_MOVE;
    } else if(board.get_piece(to_sq) != EMPTY) {
        flags = CAPTURE_MOVE;
    } else {
        flags = QUIET_MOVE;
    }

    Move move = Move(from_sq, to_sq, flags);

    assert(get_to(move) == to_sq);

    return board.move_valid(move);
}

// take back the last move in move history
void Position::take_back() {
    assert(!board_history.empty());
    board_history.pop_back();
}

bool Position::in_check() const {
    return board_history.back().in_check();
}

void Position::debug(int cnt) const {
    std::cout << RED_COLOR << "Stack trace" << RESET_COLOR << endl;
    for(int i = (int)board_history.size() - cnt; i < (int)board_history.size(); i++) {
        board_history[i].print_board();
        if(i < (int)move_history.size()) {
            Move move = static_cast<Move>(move_history[i]);
            std::cout << GREEN_COLOR << move << RESET_COLOR << endl;
        }
    }
}

bool Position::only_kings_left() const {
    uint64_t all_mask = board_history.back().get_all_mask();
    pop_first_bit(all_mask);
    pop_first_bit(all_mask);
    if(all_mask == 0)
        return true;
    return false;
}
*/