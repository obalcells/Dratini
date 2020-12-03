#include <iostream>
#include <vector>
#include "defs.h"
#include "data.h"
#include "search.h"
#include "board.h"
#include "gen.h"
#include "book.h"
#include "stats.h"
// #include "eval.h"
#include "tt.h"

Statistics stats;
TranspositionTable tt;

int main() {
	init_book();
  	tt.allocate(32);
	Position position = Position();
 	for(;;) {
		int game_result = position.game_over();
		if(game_result != -1) {
			if(game_result == WHITE) {
				std::cout << "White wins" << '\n';
			} else if(game_result == BLACK) {
				std::cout << "Black wins" << '\n';
			} else {
				std::cout << "Draw" << '\n';
			} 
   	 	}
		// std::cout << "Estimated score for white: " << eval() << " " << my_eval() << '\n';
		position.print_board();
		if(position.side == BLACK) {
			Move next_move = think(position);
			position.make_move(next_move);
			taken_moves.push_back(next_move);
			continue;
		}
		std::cout << "\nengine> ";
		std::string raw_input;
		std::cin >> raw_input;
		if(raw_input == "exit") break;
		Move move = NULL_MOVE;
		if(!parse_move(raw_input, move)) {
			std::cout << endl << "Error at parsing input `" << raw_input << "`, try again" << endl;
			continue;
	 	}
		if(position.move_valid(move)) { // move is valid
			Position prev_position = position;
			position.make_move(move);
			position = prev_position;
		} else {
			std::cout << "Invalid move '" <<  raw_input << endl;
		}
	}
}