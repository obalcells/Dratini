#include <iostream>
#include <vector>
#include "defs.h"
#include "search.h"
#include "board.h"
#include "move_picker.h"
// #include "book.h"
#include "tt.h"

TranspositionTable tt;

int main() {
	// init_book();
	tt.allocate(32);
	Board board = Board();
	
	while(true) {
		if(board.checkmate()) {
			if(board.side == WHITE) {
				board.print_board();
				cout << MAGENTA_COLOR << "Black wins" << RESET_COLOR << endl;
			} else {
				board.print_board();
				cout << MAGENTA_COLOR << "White wins" << RESET_COLOR << endl;
			}
			cout << endl << endl;
			board = Board();
			cout << GREEN_COLOR << "Starting new game" << RESET_COLOR << endl;
			continue;
		} else if(board.stalemate()) {
			board.print_board();
			cout << MAGENTA_COLOR << "Draw" << RESET_COLOR << endl;
			cout << endl << endl;
			board = Board();
			cout << GREEN_COLOR << "Starting new game" << RESET_COLOR << endl;
			continue;
		}

		if(board.side == BLACK) {
			board.print_board();
		}

		if(board.side == WHITE) {
			Move next_move = think(board);
			board.make_move(next_move);
			// board.print_board();
			// return 0;
			continue;
		}

		/*
		else {
			Move next_move = MovePicker::get_random_move(board);
			board.make_move(next_move);
			assert(board.side == BLACK);
			continue;
		}
		*/

		cout << "\nengine> ";
		std::string raw_input;
		cin >> raw_input;

		if(raw_input == "exit") {
			break;
		} else if(!board.make_move_from_str(raw_input)) { // we make the move here
			std::cout << endl << "Error at parsing input `" << raw_input << "`, try again" << endl;
			continue;
		}
	}
}
