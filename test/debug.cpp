#include <vector>
#include <random>
#include <algorithm>
#include "catch.h"
#include "../src/defs.h"
#include "../src/board.h"
#include "../src/stats.h"
#include "../src/new_position.h"
#include "../src/stats.h"
#include "../src/tt.h"
#include "../src/board.h"
#include "../src/new_position.h"
#include "../src/stats.h"
#include "../src/tt.h"

Statistics stats;
TranspositionTable tt;

int main() {
    Position position;
    NewPosition new_position;
    std::string rng_seed_str = "Dratini";
    std::seed_seq seed1 (rng_seed_str.begin(), rng_seed_str.end());
    std::vector<std::vector<std::pair<int,int> > > all_games;
	int cnt = 0;
    int move_cnt = 0;
    auto rng_shuffle = std::default_random_engine { seed1 };

	while(cnt++ < 500) {        
        position = Position();
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        move_cnt = 0;
        std::vector<std::pair<int,int> > moves;
		bool game_over = false;
		while(!game_over) {
			std::vector<std::pair<int,int> > moves_to_pick;
            for(int from_sq = 0; from_sq < 64; from_sq++) if(position.piece[from_sq] != EMPTY) { 
                for(int to_sq = 0; to_sq < 64; to_sq++) { 
                    bool old_legal, new_legal;
                    old_legal = position.move_valid(Move(from_sq, to_sq));
                    new_legal = new_position.move_valid(from_sq, to_sq);
                    assert(new_legal == old_legal);
                    if(new_legal)
                        moves_to_pick.push_back(std::make_pair(from_sq, to_sq));
                }
            }
			if(moves_to_pick.empty() || new_position.only_kings_left() || move_cnt >= 300) {
                game_over = true;
            } else {
                std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_shuffle);
                const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
                moves.push_back(moves_to_pick[0]);
                position.make_move(move);
                new_position.board_history.back().quick_check("Before making move");
                new_position.make_move_from_str(move_to_str(move));
                new_position.board_history.back().same(position);
                move_cnt++;
            }
		}
        all_games.push_back(moves);
	}
}

/*
namespace {
    std::string str_seed_ = "Dratini isn't really that fast (yet)";
    std::seed_seq seed_(str_seed.begin(), str_seed.end());
    std::mt19937_64 rng_(seed);
}

inline void same_valid_moves(Position& position, NewPosition& new_position, std::vector<std::pair<int,int> >& moves_to_pick) {
    for(int from_sq = 0; from_sq < 64; from_sq++) if(position.piece[from_sq] != EMPTY) { 
        for(int to_sq = 0; to_sq < 64; to_sq++) { 
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
			if(moves_to_pick.empty() || new_position.only_kings_left() || new_position.get_move_count() >= 500) {
				game_over = true;
			} else {
				std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_);
				const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
				position.make_move(move);
                new_position.board_history.back().quick_check("Before making move");
                // std::cout << BLUE_COLOR << "Before making move " << move_to_str(move) << RESET_COLOR << endl;
                // new_position.print_board();
				new_position.make_move_from_str(move_to_str(move));
                // std::cout << MAGENTA_COLOR << "After making move " << move_to_str(move) << RESET_COLOR << endl;
                try {
                    new_position.board_history.back().same(position);
                    new_position.board_history.back().quick_check("After making move");
                } catch(const char* error_message) {
                    std::cout << RED_COLOR << error_message << RESET_COLOR << endl;
                    position.print_board();
                    new_position.print_board();
                    std::cout << "************************" << endl;
                    new_position.debug(3);
                    assert(false);
                }
                // std::cout << GREEN_COLOR << "Quick check passed" << RESET_COLOR << endl;
			}
		}
        if(cnt % 100 == 0)
            std::cout << cnt << endl;
	}
}
*/
