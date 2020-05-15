#include <iostream>
#include "data.h"
#include "protos.h"

int main() {

	init_board();
	print_board();

	for(;;) {

		std::cout<< "\nobce> ";
		std::string raw_input;
		std::cin >> raw_input;

		if(raw_input == "exit") break;
		else {
			std::pair<int,int> processed_input = parse_move(raw_input);			

			if(processed_input.first == -1) {
				std::cout << "\nError at parsing input, try again\n";	
			} else if(make_move(processed_input.first, processed_input.second)) {
				side ^= 1;
				std::cout << '\n';
				print_board();
			} else {
				std::cout << "\nInvalid input, write a valid move in coordinate notation or exit!\n";
			}
		}
	}
}

std::pair<int,int> parse_move(std::string raw_input) {
	if((int)raw_input.size() != 4) return std::make_pair(-1, -1); //invalid

	int col_1 = raw_input[0] - 'a';
	int row_1 = raw_input[1] - '1';
	int col_2 = raw_input[2] - 'a'; 
	int row_2 = raw_input[3] - '1';

	if(!(row_1 >= 0 && row_1 < 8 && col_1 >= 0 && col_1 < 8 && row_2 >= 0 && row_2 < 8 && col_2 >= 0 && col_2 < 8)) {
		return std::make_pair(-1, -1);
	}

	return std::make_pair(row_1 * 8 + col_1, row_2 * 8 + col_2);
}
