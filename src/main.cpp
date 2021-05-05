#include <iostream>
#include <vector>
#include "defs.h"
#include "search.h"
#include "board.h"
#include "move_picker.h"
#include "sungorus_eval.h"
#include "uci.h"
#include "position.h"
#include "tt.h"
#include "engine.h"
#include "bench.h"
 
TranspositionTable tt;
Engine engine;

int main() {
	// bench();
	uci();
	return 0;
		
	engine.reset();
	Board& board = engine.board;
	bool first_input = true;

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

		if(board.side == WHITE) {
			board.print_board();
		}

		if(board.side == BLACK) {
			think(engine);
			board.make_move(engine.best_move);
			continue;
		}

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
