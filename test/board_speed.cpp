#include <sys/timeb.h>
#include <iostream>
#include <random>
#include <vector>
#include "../src/gen.h"
#include "../src/board.h"
#include "sungorus_board.h"

void test_board_speed() {
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng = std::default_random_engine { _seed };
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
    std::chrono::time_point<std::chrono::high_resolution_clock> initial_time, end_time;

    Board old_dratini_board = Board();
    double old_board_total_duration = 0.0;

    SungorusBoard sungorus_board = SungorusBoard();
    double sungorus_board_total_duration = 0.0;

    for(int game_idx = 0; game_idx < (int)1e3; game_idx++) {
        for(int move_idx = 0; move_idx < 200; move_idx++) {
            std::vector<Move> generated_moves;
            generate_moves(generated_moves, &board, false);
            if(generated_moves.size() == 0) {
                board = Board();
                break;
            }
            int move_picked = dist(rng) % int(generated_moves.size()); 
            initial_time = std::chrono::high_resolution_clock::now();
            board.make_move(generated_moves[move_picked]);
            end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end_time - initial_time;
            old_board_total_duration += duration.count();
        }
    }
    printf("Total duration for old board is: %.6lf/n", old_board_total_duration);
}
