#include <iostream>
#include "data.h"
#include "protos.h"

int main() {

	init_board();
	print_board();

	for(;;) {

		std::cout<< "\nengine> ";
		std::string raw_input;
		std::cin >> raw_input;

		if(raw_input == "exit") break;
		else {
			int from = -1, to = -1;

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
			} else {
				make_move(from, to, QUEEN);
				side ^= 1;
				std::cout << '\n';
				print_board();
			}
		}
	}
}
