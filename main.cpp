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
  std::cout << '\n';
  init_board();

  for(;;) {
    print_board();

    if(side == BLACK) {
      think(0);
      make_move(next_move.from, next_move.to, QUEEN);
      continue;
    }

    //std::cout << "Evaluation score for this state is " << eval() << endl;
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
      }

      // load snapshot
      if(int(raw_input.size()) == 1 && raw_input[0] == 'l') {
        std::string snapshot_name;
        std::cin >> snapshot_name;
        load_snapshot(snapshot_name);
        continue;
      }

      if(int(raw_input.size()) == 4) {
        int col_1 = raw_input[0] - 'a';
        int row_1 = raw_input[1] - '1';
        int col_2 = raw_input[2] - 'a';
        int row_2 = raw_input[3] - '1';

			 if(row_1 >= 0 && row_1 < 8 && col_1 >= 0 && col_1 < 8 && row_2 >= 0 && row_2 < 8 && col_2 >= 0 && col_2 < 8) {
					from = row_1 * 8 + col_1; to = row_2 * 8 + col_2;
				}
			}

      if(from == -1) {
        std::cout << "\nError at parsing input, try again\n";
        continue;
      }

      int error_code = move_valid(from, to);
      if(error_code == 0) {
        make_move(from, to, QUEEN);
        if(in_check(side)) std::cout << "IN CHECK!" << endl;
      } else {
        std::cout << "Invalid move '" <<  raw_input << "' = " << from << " -> " << to << ". Error code is " << error_code << '\n';
      }
		}
	}
}
