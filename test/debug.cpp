#include <vector>
#include <random>
#include <algorithm>
#include "../src/defs.h"
#include "../src/board.h"
#include "../src/stats.h"
#include "../src/new_position.h"
#include "../src/stats.h"
#include "../src/tt.h"

Statistics stats;
TranspositionTable tt;

namespace {
    std::string str_seed_ = "Dratini is fast!";
    std::seed_seq seed_(str_seed.begin(), str_seed.end());
    std::mt19937_64 rng_(seed);
}

inline void same_valid_moves(Position& position, NewPosition& new_position, std::vector<std::pair<int,int> >& moves_to_pick) {
    for(int from_sq = 0; from_sq < 64; from_sq++) {
        for(int to_sq = 0; to_sq < 64; to_sq++) {
            std::cout << "Checking move valid" << endl;
            bool old_legal = position.move_valid(Move(from_sq, to_sq));
            bool new_legal = new_position.move_valid(from_sq, to_sq);
            if(new_legal != old_legal) {
                std::cout << RED_COLOR << "Move is " << move_to_str(Move(from_sq, to_sq)) << RESET_COLOR << endl;            
                std::cout << "Old board: (" << (old_legal ? "valid" : "not valid") << ")" << endl;
                position.print_board();
                std::cout << "New board: (" << (new_legal ? "valid" : "not valid") << ")" << endl;
                new_position.print_board();
				// new_position.debug();
            }
            assert(new_legal == old_legal);
            if(new_legal)
                moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
            std::cout << "Finished checking move valid" << endl;
        }
    }
}

int main() {
    Position position = Position();
    NewPosition new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	int cnt = 0;
	while(cnt++ < 10000) {
        position = Position();
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		bool game_over = false;
		while(!game_over) {
			std::vector<std::pair<int,int> > moves_to_pick;
			same_valid_moves(position, new_position, moves_to_pick);
			if(moves_to_pick.empty()) {
				game_over = true;
			} else {
				std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_);
				const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
				position.make_move(move);
				new_position.make_move_from_str(move_to_str(move));
			}
		}
        std::cout << "Finished game" << endl;
	}
}
