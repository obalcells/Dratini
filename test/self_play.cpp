#include <cstdlib>
#include <algorithm>
#include <string>

#include "../src/data.h"
#include "../src/board.h"
#include "../src/search.h"
#include "../src/stats.h"
#include "../src/tt.h"

Statistics stats;
TranspositionTable tt;

int main() {
  	tt.allocate(32);
    Position position;
    std::string printed_progress = "[";
    std::cout << '\n' << '[';
    std::cout.flush();
    int n_errors = 0;
    int n_games = 100;
    for(int game_idx = 0; game_idx < n_games; game_idx++) {
        position.init_board();
        while(!taken_moves.empty()) taken_moves.pop_back();
        tt.clear();
        try {
            while(position.game_over() == -1) {
                Move best_move = think(position); 
                taken_moves.push_back(best_move);
                position.make_move(best_move);
            }
        } catch(const char* msg) {
            std::cerr << '\n' << '\n' << "There was an error: " << RED_COLOR << msg << '\n' << '\n' << RESET_COLOR;
            std::cerr << "Move stack is: ";
            std::vector<Move> tmp_taken_moves = taken_moves; // we make a copy because it will be deleted
            position.init_board();
            position.print_board();
            for(int move_idx = 0; move_idx < (int)tmp_taken_moves.size(); move_idx++) {
                std::cerr << str_move(tmp_taken_moves[move_idx].from, tmp_taken_moves[move_idx].to) << '\n'; 
                position.make_move(tmp_taken_moves[move_idx]);
                position.print_board();
            }
            std::cerr << RED_COLOR << "*****************************************************" << '\n' << '\n' << RESET_COLOR;
            std::cout << printed_progress; 
            std::cout.flush();
            n_errors++;
        }
        if(game_idx) { // && game_idx % 10 == 0) {
            printed_progress += '=';
            std::cout << '=';
            std::cout.flush();
        }
    }
    std::cout << "]" << '\n' << '\n';
    if(n_errors == 0) {
        std::cout << GREEN_COLOR << "No errors" << '\n' << '\n' << RESET_COLOR; 
    } else {
        std::cout << RED_COLOR << n_errors << " errors in " << n_games << " games" << '\n' << '\n' << RESET_COLOR; 
    }
}