#include <iostream>
#include <vector>
#include "defs.h"
#include "search.h"
#include "board.h"
#include "move_picker.h"
#include "uci.h"
#include "position.h"
#include "tt.h"
#include "engine.h"

TranspositionTable tt;

int main() {
	uci();
	return 0;

	// init_book();
	tt.allocate(32);
		
	Engine engine = Engine("rn3k1r/p3bppp/3p3n/1p6/B5b1/B1q1RNP1/P1P2PK1/1R1Q4 b - - 0 1");
	// Engine engine = Engine("rnb1k1nr/pppqbppp/3pp3/1B6/3PP3/2N2N2/PPP2PPP/R1BQK2R b KQkq - 0 1");
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
