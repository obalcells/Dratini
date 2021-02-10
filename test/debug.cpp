#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>

#include "../src/defs.h"
#include "../src/new_position.h"
#include "../src/new_board.h"
#include "../src/board.h"
#include "../src/stats.h"
#include "../src/tt.h"
#include "../src/data.h"
#include "../src/gen.h"
#include "../src/new_gen.h"
#include "../src/move_picker.h"

Statistics stats;
TranspositionTable tt;

int main() {
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { _seed };
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

    NewPosition position;
    NewMove tmp_move;
	int games_played = 0;

    while(games_played++ < 1000) {
        position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        bool game_over = false;
        int move_cnt = 0;
         
        while(!game_over && move_cnt < 100) {
            move_cnt++;
            MovePicker move_picker(position.get_board());

            try {
                tmp_move = move_picker.next_move();
                assert(position.move_valid(tmp_move) == true);
                position.make_move(tmp_move);
            } catch(const std::string error_msg) {
                std::cerr << RED_COLOR << "There was an error: " << error_msg << RESET_COLOR << endl; 
                std::cerr << "Position currently is:" << endl;
                position.print_board();
                assert(false);
            }
        }
    }
}

// we use this to test specific positions
int __main() {
    NewPosition position = NewPosition("./games/bad_see_queen_a8f8.txt", true);
    MovePicker move_picker(position.get_board());
    NewMove tmp_move = move_picker.next_move();
}

/*
bool operator<(const NewMove& a, const NewMove& b) {
    if(a.get_from() != b.get_from())
        return a.get_from() < b.get_from();
    return a.get_to() < b.get_to();
}

bool operator==(const NewMove& a, const Move& b) {
    return a.get_from() == from(b) && a.get_to() == to(b);
}

bool operator!=(const NewMove& a, const Move& b) {
    return !(a == b);
}

int main() {
    std::string rng_seed_str = "Dratini";
    std::seed_seq seed1 (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { seed1 };

    Position position;
    NewPosition new_position;
    std::vector<NewMove> new_move_stack;
	int games_played = 0;

    while(games_played++ < 10000) {
        position = Position();
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        bool game_over = false;
         
        while(!game_over) {
            move_stack.clear();
            new_move_stack.clear();
            generate_moves(position);
            generate_moves(new_move_stack, new_position.get_board()); 

            sort(new_move_stack.begin(), new_move_stack.end());

            if(move_stack.size() != new_move_stack.size()) {
                std::cout << RED_COLOR << "Number of moves is different" << RESET_COLOR << endl;
            }

            bool error_gen = false;

            for(int i = 0; i < (int)std::min(new_move_stack.size(), move_stack.size()); i++) {
                if(new_move_stack[i] != move_stack[i]) {
                    error_gen = true;
                }    
            }

            if(new_move_stack.size() != move_stack.size())
                error_gen = true;

            if(error_gen) {
                std::cout << "Board is:" << endl;
                new_position.print_board();
                std::cout << BLUE_COLOR << "Moves only in OLD move generation:" << RESET_COLOR << endl;
                for(int i = 0; i < (int)move_stack.size(); i++) {
                    bool found = false;
                    for(int j = 0; !found && j < (int)new_move_stack.size(); j++) {
                        if(new_move_stack[j] == move_stack[i])
                            found = true;     
                    }
                    if(!found) {
                        std::cout << move_to_str(move_stack[i]) << " ";
                    }
                }
                std::cout << endl;
                std::cout << GREEN_COLOR << "Moves only in NEW move generation" << endl;
                for(int i = 0; i < (int)new_move_stack.size(); i++) {
                    bool found = false;
                    for(int j = 0; !found && j < (int)move_stack.size(); j++) {
                        if(new_move_stack[i] == move_stack[j])
                            found = true;     
                    }
                    if(!found) {
                        std::cout 
                        << move_to_str(Move(new_move_stack[i].get_from(), new_move_stack[i].get_to()))
                        << " ";
                    }
                }
                std::cout << endl;
                assert(error_gen == false);
            }

			if(move_stack.empty() 
            || position.move_cnt >= 300) {
                game_over = true;
            } else {
                std::shuffle(move_stack.begin(), move_stack.end(), rng_shuffle);
                position.make_move(move_stack[0]);
                new_position.make_move_from_str(move_to_str(move_stack[0]));
            }
        }
    }
}

int main() {
    std::string rng_seed_str = "Dratini";
    std::seed_seq seed1 (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { seed1 };
    Position position;
    NewPosition new_position;
    std::vector<std::vector<std::pair<int,int> > > all_games;
	int games_played = 0;

	while(games_played++ < 500) {        
        position = Position();
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
			if(moves_to_pick.empty() 
            || new_position.only_kings_left() 
            || new_position.board_history.back().move_count >= 300) {
                game_over = true;
            } else {
                std::shuffle(moves_to_pick.begin(), moves_to_pick.end(), rng_shuffle);
                const Move move = Move(moves_to_pick[0].first, moves_to_pick[0].second);
                moves.push_back(moves_to_pick[0]);
                position.make_move(move);
                new_position.board_history.back().quick_check("Before making move");
                new_position.make_move_from_str(move_to_str(move));
                new_position.board_history.back().same(position);
            }
		}
        all_games.push_back(moves);
	}
}
*/
