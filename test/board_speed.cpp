#include <sys/timeb.h>
#include <iostream>
#include <random>
#include <vector>
#include <iomanip>
#include "../src/gen.h"
#include "../src/board.h"

void test_make_move_speed() {
	std::string rng_seed_str = "Dratini";
	std::seed_seq _seed(rng_seed_str.begin(), rng_seed_str.end());
	auto rng = std::default_random_engine {
		_seed
	};
	std::uniform_int_distribution < uint64_t > dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
	std::chrono::time_point < std::chrono::high_resolution_clock > initial_time, end_time;
	std::chrono::duration < double, std::milli > duration;
	std::vector < Move > moves;
	Board first_board = Board();
	Board board = Board();
	Board new_board = Board();
	UndoData _undo_data = UndoData(new_board.king_attackers);

	double first_duration = 0.0;
	double old_duration = 0.0;
	double new_duration = 0.0;

	for (int game_idx = 0; game_idx < (int) 4e4; game_idx++) {
		first_board = board = new_board = Board();
		moves.clear();

		// we generate the list of moves
		for (int move_idx = 0; move_idx < 200; move_idx++) {
			std::vector < Move > generated_moves;
			generate_moves(generated_moves, & first_board, false);
			if (generated_moves.empty() || first_board.fifty_move_ply >= 50)
				break;
			int move_picked = dist(rng) % int(generated_moves.size());
			Move move = generated_moves[move_picked];
			assert(first_board.move_valid(move));
			moves.push_back(move);
			first_board.make_move(move, _undo_data);
		}

		// first board (for some reason the board that runs first takes more time)
		first_board = Board();
		initial_time = std::chrono::high_resolution_clock::now();
		for (int move_idx = 0; move_idx < moves.size(); move_idx++) {
			first_board.make_move(moves[move_idx], _undo_data);
		}
		end_time = std::chrono::high_resolution_clock::now();
		duration = end_time - initial_time;
		first_duration += duration.count();

		// dev
		initial_time = std::chrono::high_resolution_clock::now();
		for (int move_idx = 0; move_idx < moves.size(); move_idx++) {
			new_board.new_make_move(moves[move_idx], _undo_data);
		}
		end_time = std::chrono::high_resolution_clock::now();
		duration = end_time - initial_time;
		new_duration += duration.count();

		// base
		initial_time = std::chrono::high_resolution_clock::now();
		for (int move_idx = 0; move_idx < moves.size(); move_idx++) {
			board.make_move(moves[move_idx], _undo_data);
		}
		end_time = std::chrono::high_resolution_clock::now();
		duration = end_time - initial_time;
		old_duration += duration.count();
	}

	printf("Total duration for FIRST board is: %lf\n", first_duration);
	printf("Total duration for OLD   board is: %lf\n", old_duration);
	printf("Total duration for NEW   board is: %lf\n", new_duration);
}

void test_move_valid_speed() {
	std::string rng_seed_str = "Dratini!";
	std::seed_seq _seed(rng_seed_str.begin(), rng_seed_str.end());
	auto rng = std::default_random_engine { _seed };
	std::uniform_int_distribution < uint64_t > dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
	std::chrono::time_point < std::chrono::high_resolution_clock > initial_time, end_time;
	std::vector<Move> moves;
	Board board = Board();
	UndoData _undo_data = UndoData(board.king_attackers);
	std::vector<Move> raw_moves, valid_moves;
	int move_picked;
	Move move;

	double old_mar_duration = 0.0;
	double new_mar_duration = 0.0;
	double new_mar_and_mm_duration = 0.0;
	double movevalid_duration = 0.0;
	double new_movevalid_duration = 0.0;
	double fast_duration = 0.0;
	double old_mar_control_duration = 0.0;

	int n_games = 50000, game_repetitions = 1;
	long long fingerprint = 0, prev_fingerprint, fingerprint_diff;
	uint64_t prev_king_attackers;

	for(int game_idx = 0; game_idx < n_games; game_idx++) {
		board = Board();

		for(int move_idx = 0; move_idx < 200; move_idx++) {
			raw_moves.clear();
			valid_moves.clear();

			generate_captures(raw_moves, &board);
			generate_quiet(raw_moves, &board);

            for(int j = 0; j < game_repetitions; j++) {
				prev_fingerprint = fingerprint;

                // Old MAR 
				// prev_king_attackers = board.king_attackers;
                // initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                    board.new_make_move(raw_moves[i], _undo_data);
					fingerprint += !board.opp_king_attacked();
					// fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
                    board.new_take_back(_undo_data);
                }
                // old_mar_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				fingerprint_diff = fingerprint - prev_fingerprint;
				prev_fingerprint = fingerprint;

                // New MAR 
				// prev_king_attackers = board.king_attackers;
                // initial_time = std::chrono::high_resolution_clock::now();
                // for(int i = 0; i < raw_moves.size(); i++) {
                // 	board.make_move(raw_moves[i], _undo_data);
				// 	// fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
				// 	if(!board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side)) {
				// 		fingerprint++; // += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
				// 		// cerr << GREEN_COLOR << " " << move_to_str(raw_moves[i]) << RESET_COLOR << endl;
				// 	} else {
				// 		// cerr << RED_COLOR << " " << move_to_str(raw_moves[i]) << RESET_COLOR << endl;
				// 	}
                //     board.new_take_back(_undo_data);
				// 	board.king_attackers = prev_king_attackers;
                // }
                // new_mar_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				// assert(fingerprint_diff == fingerprint - prev_fingerprint);
				// fingerprint_diff = fingerprint - prev_fingerprint;
				// prev_fingerprint = fingerprint;

                // New MAR 
				// prev_king_attackers = board.king_attackers;
                // initial_time = std::chrono::high_resolution_clock::now();
                // for(int i = 0; i < raw_moves.size(); i++) {
                // 	board.new_make_move(raw_moves[i], _undo_data);
				// 	if(!board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side)) {
				// 		fingerprint++; // += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
				// 		// cerr << GREEN_COLOR << " " << move_to_str(raw_moves[i]) << RESET_COLOR << " " << fingerprint << endl;
				// 	} else {
				// 		// cerr << RED_COLOR << " " << move_to_str(raw_moves[i]) << RESET_COLOR << endl;
				// 	}
                //     board.new_take_back(_undo_data);
				// 	board.king_attackers = prev_king_attackers;
                // }
                // new_mar_and_mm_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				// assert(fingerprint_diff ==fingerprint - prev_fingerprint);
				// fingerprint_diff = fingerprint - prev_fingerprint;
				// prev_fingerprint = fingerprint;

                // // Move valid 
                // initial_time = std::chrono::high_resolution_clock::now();
                // for(int i = 0; i < raw_moves.size(); i++) {
				// 	if(board.move_valid(raw_moves[i])) {
				// 		fingerprint++;
				// 		board.make_move(raw_moves[i], _undo_data);
				// 		board.take_back(_undo_data);
				// 	}
                // }
                // movevalid_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				// assert(fingerprint_diff == fingerprint - prev_fingerprint);
				// fingerprint_diff = fingerprint - prev_fingerprint;
				// prev_fingerprint = fingerprint;

                // New Move valid 
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
					if(board.new_move_valid(raw_moves[i]) && board.fast_move_valid(raw_moves[i])) {  
						fingerprint++;
						board.make_move(raw_moves[i], _undo_data);
						board.take_back(_undo_data);
					}
                }
                // new_movevalid_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == fingerprint - prev_fingerprint);
				fingerprint_diff = fingerprint - prev_fingerprint;
				prev_fingerprint = fingerprint;

				// Fast
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                    if(board.fast_move_valid(raw_moves[i])) {
						// if(!board.move_valid(raw_moves[i])) {
						// 	cerr << "Move " << move_to_str(raw_moves[i]) << " isn't valid but fmv says it is" << endl;
						// 	cerr << "Board looks like this:" << endl;
						// 	board.print_board();
						// 	assert(false);
						// }
						fingerprint++;
                        valid_moves.push_back(raw_moves[i]);
						board.new_make_move(raw_moves[i], _undo_data);
						board.new_take_back(_undo_data);
					} else {
						// if(board.move_valid(raw_moves[i])) {
						// 	cerr << "Move " << move_to_str(raw_moves[i]) << " is valid but fmv says it isnt" << endl;
						// 	cerr << "Board looks like this:" << endl;
						// 	board.print_board();
						// 	assert(false);
						// }
					} 
                }
                // fast_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == fingerprint - prev_fingerprint);
				fingerprint_diff = fingerprint - prev_fingerprint;
				prev_fingerprint = fingerprint;

                // Dratini Old MAR (control)
				// prev_king_attackers = board.king_attackers;
                // initial_time = std::chrono::high_resolution_clock::now();
                // for(int i = 0; i < raw_moves.size(); i++) {
                //     board.make_move(raw_moves[i], _undo_data);
				// 	fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
                //     board.take_back(_undo_data);
                // }
                // old_mar_control_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				// assert(fingerprint_diff == fingerprint - prev_fingerprint);
				// prev_fingerprint = fingerprint;
            }

			// we pick a random move and apply it to all the boards
			if(valid_moves.empty()) break;
			move_picked = dist(rng) % int(valid_moves.size());
			board.new_make_move(valid_moves[move_picked], _undo_data);
		}
	}

	// n_games *= game_repetitions * 2 * 100;

	// cout << "Per game duration:" << endl;
	// cout << "Old MAR   " << std::setw(12) << int(old_mar_duration / double(n_games)) << endl; 
	// cout << "New MAR   " << std::setw(12) << int(new_mar_duration / double(n_games)) << endl; 
	// cout << "New MARMM " << std::setw(12) << int(new_mar_and_mm_duration / double(n_games)) << endl; 
	// cout << "M valid   " << std::setw(12) << int(movevalid_duration / double(n_games)) << endl; 
	// cout << "Fast      " << std::setw(12) << int(fast_duration / double(n_games)) << endl; 
	// cout << "OLD MAR C " << std::setw(12) << int(old_mar_control_duration / double(n_games)) << endl; 
	cout << "Fingerprint " << fingerprint << endl;
	// cout << "Fingerprint should be " << 0 << endl;
}

void check_occ(Board& board) {
	uint64_t occ_mask = 0;
	for(int i = 0; i < 12; i++) {
		occ_mask |= board.bits[i];
	}
	assert(board.occ_mask == occ_mask);
}

void test_see_speed() {
	std::string rng_seed_str = "Dratini";
	std::seed_seq _seed(rng_seed_str.begin(), rng_seed_str.end());
	auto rng = std::default_random_engine {
		_seed
	};
	std::uniform_int_distribution < uint64_t > dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
	std::chrono::time_point < std::chrono::high_resolution_clock > initial_time, end_time;
	std::vector<Move> raw_moves, valid_moves;
	Board board = Board();
	UndoData _undo_data = UndoData(board.king_attackers);
	int score, move_picked;
	Move move;

	long long dratini_see = 0;

	// constants
	int n_games = (int)1e6;
    const int game_repetitions = 1;

	for(int game_idx = 0; game_idx < n_games; game_idx++) {
		board = Board();

		for(int move_idx = 0; move_idx < 200; move_idx++) {
			raw_moves.clear();
			valid_moves.clear();
			raw_moves.reserve(64);
			valid_moves.reserve(64);

			generate_captures(raw_moves, &board);
			generate_quiet(raw_moves, &board);

			for(int i = 0; i < raw_moves.size(); i++) {
				if(board.fast_move_valid(raw_moves[i])) {
					valid_moves.push_back(raw_moves[i]);
				}
			}

			initial_time = std::chrono::high_resolution_clock::now();
			for(int j = 0; j < game_repetitions; j++) {
				for(int i = 0; i < valid_moves.size(); i++) if(get_flag(valid_moves[i]) == CAPTURE_MOVE) { 
					board.fast_see(valid_moves[i]);	
				}	
			}
			dratini_see += (long long)(std::chrono::high_resolution_clock::now() - initial_time).count();

			// we pick a random move and apply it to all the boards
			if(valid_moves.empty()) break;
			move_picked = dist(rng) % int(valid_moves.size());
			move = valid_moves[move_picked];
			board.make_move(valid_moves[move_picked], _undo_data);
		}		
	}

	n_games *= game_repetitions;

	cout << "Per game duration:" << endl;
	cout << "Dratini    " << std::setw(10) << int(dratini_see / double(n_games)) / 100 << endl; 
}