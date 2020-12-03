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
        std::vector<Move> tmp_taken_moves;
        try {
            bool move_now = WHITE;
            while(position.game_over() == -1) {
                assert(position.side == move_now);
                Move best_move = think(position); 
                assert(position.move_valid(best_move));
                tmp_taken_moves.push_back(best_move);
                position.make_move(best_move);
                move_now = !move_now;
            }
        } catch(const char* msg) {
            std::cerr << endl << endl << "There was an error: " << RED_COLOR << msg << RESET_COLOR << endl << endl;
            position.init_board();
            position.print_board();
            for(int move_idx = 0; move_idx < (int)tmp_taken_moves.size(); move_idx++) {
                std::cerr << move_to_str(tmp_taken_moves[move_idx]) << '\n'; 
                assert(position.move_valid(tmp_taken_moves[move_idx]) == true);
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
    std::cout << "]" << endl << endl;
    if(n_errors == 0) {
        std::cout << GREEN_COLOR << "0 errors in " << n_games << '\n' << '\n' << RESET_COLOR; 
    } else {
        std::cout << RED_COLOR << n_errors << " errors in " << n_games << " games" << '\n' << '\n' << RESET_COLOR; 
    }
}