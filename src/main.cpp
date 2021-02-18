#include <iostream>
#include <vector>
#include "defs.h"
#include "search.h"
#include "position.h"
#include "board.h"
// #include "book.h"
#include "tt.h"

TranspositionTable tt;

int main() {
	cerr << "Hi!" << endl;

	// init_book();
	tt.allocate(32);
	cerr << "Finished allocating" << endl;
	Position position = Position();
	cerr << "Initalized board" << endl;
	
	while(true) {
		if(position.get_board().side == WHITE) {
			position.print_board();
		}

		if(position.get_board().checkmate()) {
			if(position.get_board().side == WHITE) {
				cout << MAGENTA_COLOR << "Black wins" << RESET_COLOR << endl;
			} else {
				cout << MAGENTA_COLOR << "White wins" << RESET_COLOR << endl;
			}
			cout << endl << endl;
			position = Position();
			continue;
		} else if(position.get_board().stalemate()) {
			cout << MAGENTA_COLOR << "Draw" << RESET_COLOR << endl;
			cout << endl << endl;
			position = Position();
			continue;
		}

		if(position.get_board().side == BLACK) {
			cerr << MAGENTA_COLOR << "Side when starting to think is " << (position.get_board().side == WHITE ? "WHITE" : "BLACK") << RESET_COLOR << endl;
			Move next_move = think(position);
			cerr << "Finished thinking" << endl;
			position.make_move(next_move);
			continue;
		}

		cout << "\nengine> ";
		std::string raw_input;
		cin >> raw_input;

		if(raw_input == "exit") {
			break;
		} else if(!position.make_move_from_str(raw_input)) { // we make the move here
			std::cout << endl << "Error at parsing input `" << raw_input << "`, try again" << endl;
			continue;
		} else {
			cerr << "Side after making move is " << (position.get_board().side == WHITE ? "WHITE" : "BLACK") << endl;
		}
	}
}
