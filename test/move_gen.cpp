#include <vector>
#include <random>
#include <algorithm>
#include "catch.h"
#include "../src/defs.h"
#include "../src/new_position.h"
#include "../src/new_board.h"
#include "../src/board.h"
#include "../src/data.h"
#include "../src/new_gen.h"
#include "../src/gen.h"

bool operator<(const NewMove& a, const NewMove& b) {
    if(a.get_from() != b.get_from())
        return a.get_from() < b.get_from();
    return a.get_to() < b.get_to();
}

bool operator==(const NewMove& a, const NewMove& b) {
    return a.get_from() == b.get_from() && a.get_to() == b.get_to();
}

bool operator!=(const NewMove& a, const NewMove& b) {
    return !(a == b);
}

TEST_CASE("Move generation check for correctness") {
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { _seed };
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

    NewPosition position;
    std::vector<NewMove> fast_move_stack, slow_move_stack;
	int games_played = 0;
    NewMove tmp_move;

    while(games_played++ < 100) {
        position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        bool game_over = false;
        int move_cnt = 0;
         
        while(!game_over) {
            move_cnt++;
            fast_move_stack.clear();
            slow_move_stack.clear();

            for(int from_sq = 0; from_sq < 64; from_sq++) {
            	if(position.get_board().get_piece(from_sq) != NEW_EMPTY) {
	            	for(int to_sq = 0; to_sq < 64; to_sq++) {
	            		tmp_move = position.pair_to_move(from_sq, to_sq);
	            		if(position.move_valid(tmp_move))
		            		slow_move_stack.push_back(tmp_move);
	            	}
            	}
            }

            generate_moves(fast_move_stack, position.get_board()); 
            sort(fast_move_stack.begin(), fast_move_stack.end());

            bool error_gen = false;

            if(fast_move_stack.size() != slow_move_stack.size())
                error_gen = true;

            for(int i = 0; i < (int)std::min(fast_move_stack.size(), slow_move_stack.size()); i++) {
                if(fast_move_stack[i] != slow_move_stack[i]) {
                    error_gen = true;
                }    
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

                std::cerr << endl;
                std::cerr << GREEN_COLOR << "Moves only in NEW move generation" << endl;

                for(int i = 0; i < (int)fast_move_stack.size(); i++) {
                    bool found = false;
                    for(int j = 0; !found && j < (int)slow_move_stack.size(); j++) {
                        if(fast_move_stack[i] == slow_move_stack[j])
                            found = true;     
                    }
                    if(!found)
                    	std::cerr << move_to_str(fast_move_stack[i]) << " ";
                }
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

TEST_CASE("Benchmark move generation") {
    /* for the random number generator */
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng_shuffle = std::default_random_engine { _seed };
    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));

    /* local helper variables */
    std::vector<NewMove> fast_move_stack, slow_move_stack;
    NewPosition new_position;
    Position position;
    int games_played = 0;

    games_played = 0;
    // rng_shuffle = std::default_random_engine { _seed };

    BENCHMARK("Brute-force move generation")  {
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        bool game_over = false;
        NewMove tmp_move;
         
        while(!game_over) {
            slow_move_stack.clear();
            for(int from_sq = 0; from_sq < 64; from_sq++) {
                if(new_position.get_board().get_piece(from_sq) != NEW_EMPTY) {
                    for(int to_sq = 0; to_sq < 64; to_sq++) {
                        tmp_move = new_position.pair_to_move(from_sq, to_sq);
                        if(new_position.move_valid(tmp_move)) {
                            slow_move_stack.push_back(tmp_move);
                        }
                    }
                }
            }

            if(slow_move_stack.empty() || new_position.get_move_count() >= 300) {
                game_over = true;
            } else {
                int move_idx = dist(rng_shuffle) % (int)slow_move_stack.size();
                new_position.make_move(slow_move_stack[move_idx]);
            }
        }
    };

    rng_shuffle = std::default_random_engine { _seed };
    games_played = 0;

    BENCHMARK("Old move generation (with bugs)") {
        position = Position();
        bool game_over = false;
         
        while(!game_over) {
            move_stack.clear();
            generate_moves(position);

            if(move_stack.empty() || position.move_cnt >= 300) {
                game_over = true;
            } else {
                int move_idx = dist(rng_shuffle) % (int)move_stack.size();
                position.make_move(move_stack[move_idx]);
            }
        }
    };

    rng_shuffle = std::default_random_engine { _seed };
    games_played = 0;

    BENCHMARK("New move generation") {
        new_position = NewPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        bool game_over = false;
         
        while(!game_over) {
            fast_move_stack.clear();
            generate_moves(fast_move_stack, new_position.get_board()); 

            if(fast_move_stack.empty() || new_position.get_move_count() >= 300) {
                game_over = true;
            } else {
                int move_idx = dist(rng_shuffle) % (int)fast_move_stack.size();
                new_position.make_move(fast_move_stack[move_idx]);
            }
        }
    };
}
