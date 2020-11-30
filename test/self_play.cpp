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
        tt.clear();
        try {
            while(position.game_over() == -1) {
                Move best_move = think(position); 
                position.make_move(best_move);
            }
        } catch(const char* msg) {
            std::cerr << "There was an error: " << RED_COLOR << msg << '\n' << '\n' << RESET_COLOR;
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