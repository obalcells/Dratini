#include <vector>
#include <random>
#include <algorithm>
#include "catch.h"

#include "../src/defs.h"
#include "../src/board.h"
#include "../src/gen.h"
#include "../src/move_picker.h"

TEST_CASE("Sorting captures") {
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { _seed };
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

    Position position;
    Move tmp_move;
	int games_played = 0;

    BENCHMARK("Slow SEE") {
        while(games_played++ < 200) {
            position = Position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            bool game_over = false;
            int move_cnt = 0;

            
            while(!game_over && move_cnt < 100) {
                move_cnt++;
                // MovePicker slow_move_picker(position.get_board(), true);
                MovePicker slow_move_picker(position.get_board());

                try {
                    tmp_move = slow_move_picker.next_move();
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
    };

    BENCHMARK("Fast SEE") {
        while(games_played++ < 200) {
            position = Position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
    };
}


TEST_CASE("Move generation check for correctness") {
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { _seed };
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

    Position position;
    std::vector<Move> fast_move_stack, slow_move_stack;
    Move tmp_move;
	int games_played = 0;

    while(games_played++ < 100) {
        position = Position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        bool game_over = false;
        int move_cnt = 0;
         
        while(!game_over) {
            move_cnt++;
            fast_move_stack.clear();
            slow_move_stack.clear();

            for(int from_sq = 0; from_sq < 64; from_sq++) {
            	if(position.get_board().get_piece(from_sq) != EMPTY) {
	            	for(int to_sq = 0; to_sq < 64; to_sq++) {
	            		tmp_move = position.pair_to_move(from_sq, to_sq);
	            		if(position.move_valid(tmp_move))
		            		slow_move_stack.push_back(tmp_move);
	            	}
            	}
            }

            generate_moves(fast_move_stack, &position.get_board()); 
            sort(slow_move_stack.begin(), slow_move_stack.end());
            sort(fast_move_stack.begin(), fast_move_stack.end());

            bool error_gen = false;

            REQUIRE(fast_move_stack.size() == slow_move_stack.size());

            if(fast_move_stack.size() != slow_move_stack.size()) {
                error_gen = true;
            }

            for(int i = 0; i < (int)std::min(fast_move_stack.size(), slow_move_stack.size()); i++) {
                if(fast_move_stack[i] != slow_move_stack[i]) {
                    std::cerr << "Fast move stack has " << move_to_str(fast_move_stack[i]) << endl;
                    std::cerr << "Slow move stack has " << move_to_str(slow_move_stack[i]) << endl;
                    error_gen = true;
                }
                REQUIRE(fast_move_stack[i] == slow_move_stack[i]);
            }

            if(error_gen) {
                std::cerr << "Board is:" << endl;
                position.print_board();
                std::cerr << BLUE_COLOR << "Moves only in SLOW move generation:" << RESET_COLOR << endl;

                for(int i = 0; i < (int)slow_move_stack.size(); i++) {
                    bool found = false;
                    for(int j = 0; !found && j < (int)fast_move_stack.size(); j++) {
                        if(slow_move_stack[i] == fast_move_stack[j])
                            found = true;
                    }
                    if(!found)
                        std::cerr << move_to_str(slow_move_stack[i]) << " ";
                }

                std::cerr << "***" << endl;
                std::cerr << endl;
                std::cerr << GREEN_COLOR << "Moves only in NEW move generation" << endl;

                for(int i = 0; i < (int)fast_move_stack.size(); i++) {
                    bool found = false;
                    for(int j = 0; !found && j < (int)slow_move_stack.size(); j++) {
                        if(fast_move_stack[i] == slow_move_stack[j])
                            found = true;     
                    }
                    if(!found) {
                    	std::cerr << move_to_str(fast_move_stack[i]) << " ";
                    }
                }
                std::cerr << "***" << endl;
                std::cerr << endl;
            }

            REQUIRE(error_gen == false);

			if(fast_move_stack.empty() || position.get_move_count() >= 300) {
                game_over = true;
            } else {
            	int move_idx = dist(rng_shuffle) % (int)fast_move_stack.size();
                position.make_move(fast_move_stack[move_idx]);
            }
        }
    }
}
