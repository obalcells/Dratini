#include <iostream>
#include <vector>
#include "defs.h"
#include "data.h"
#include "protos.h"
#include "search.h"
#include "board.h"
#include "move.h"
#include "gen.h"
// #include "eval.h"

int main() {
  init_board();

  for(;;) {
    int game_result = game_over();

    if(game_result != -1) {
      if(game_result == WHITE) {
        std::cout << "White wins" << '\n';
      } else if(game_result == BLACK) {
        std::cout << "Black wins" << '\n';
      } else {
        std::cout << "Draw" << '\n';
      } 
    }

    print_board();

    if(side == BLACK) {
      think(0);
      Move move = make_move(next_move.from, next_move.to, QUEEN);
      taken_moves.push_back(move);
      continue;
    }

    std::cout << "\nengine> ";
    std::string raw_input;
    std::cin >> raw_input;

    if(raw_input == "exit") break;
    else {
      int from = -1, to = -1;

      // save snapshot
      if(int(raw_input.size()) == 1 && raw_input[0] == 's') {
        std::string snapshot_name;
        std::cin >> snapshot_name;
        save_snapshot(snapshot_name);
        continue;
      }

      // load snapshot
      if(int(raw_input.size()) == 1 && raw_input[0] == 'l') {
        std::string snapshot_name;
        std::cin >> snapshot_name;
        load_snapshot(snapshot_name);
        std::cout << "Side is now: " << (side == WHITE ? "WHITE" : "BLACK") << endl;
        continue;
      }
  
      if(!parse_move(raw_input, from, to)) {
        std::cout << endl << "Error at parsing input `" << raw_input << "`, try again" << endl;
        continue;
      }

      int error_code = move_valid(from, to);
      if(error_code == 0) { // move is valid
        Move move = make_move(from, to, QUEEN);
        taken_moves.push_back(move);
        if(in_check(side)) std::cout << "IN CHECK!" << endl;
      } else {
        std::cout << "Invalid move '" <<  raw_input << "' = " << from << " -> " << to << ". Error code is " << error_code << '\n';
      }
		}
	}
}

// the computer plays against itself to find errors quicker
void test() {
	init_board();	

	for(;;) {
		int game_result = game_over();
		
		if(game_result != -1) {
		if(game_result == WHITE) {
			test(); // recursively executed
		} else if(game_result == BLACK) {
			test();
		} else {
			test();
		} 
	}

	think(0);
	Move move = make_move(next_move.from, next_move.to, QUEEN);
	taken_moves.push_back(move);
  }
}
