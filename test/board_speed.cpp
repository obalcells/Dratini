#include <sys/timeb.h>
#include <iostream>
#include <random>
#include <vector>
#include <iomanip>
#include "../src/gen.h"
#include "../src/board.h"
#include "sungorus_board.h"

int StrToMove(POS * p, char * move_str) {
	int from, to, type;

	from = Sq(move_str[0] - 'a', move_str[1] - '1');
	to = Sq(move_str[2] - 'a', move_str[3] - '1');
	type = NORMAL;
	if (TpOnSq(p, from) == K && Abs(to - from) == 2)
		type = CASTLE;
	else if (TpOnSq(p, from) == P) {
		if (to == p -> ep_sq)
			type = EP_CAP;
		else if (Abs(to - from) == 16)
			type = EP_SET;
		else if (move_str[4] != '\0')
			switch (move_str[4]) {
			case 'n':
				type = N_PROM;
				break;
			case 'b':
				type = B_PROM;
				break;
			case 'r':
				type = R_PROM;
				break;
			case 'q':
				type = Q_PROM;
				break;
			}
	}
	return (type << 12) | (to << 6) | from;
}

bool check_same(POS * p, Board * board) {
	for (int i = 0; i < 64; i++) {
		if (p -> pc[i] == NO_PC && board -> piece_at[i] != EMPTY)
			return false;
		if (p -> pc[i] != NO_PC && board -> piece_at[i] == EMPTY)
			return false;
		if (p -> pc[i] == NO_PC && board -> piece_at[i] == EMPTY)
			continue;
		int p1 = (p -> pc[i] >> 1) + ((p -> pc[i] & 1) ? 6 : 0);
		int p2 = board -> piece_at[i] + (board -> color_at[i] == BLACK ? 6 : 0);
		if (p1 != p2) {
			cerr << "At position " << i << " pieces are not the same" << endl;
			cerr << (int) p -> pc[i] << " " << (int) board -> piece_at[i] << " " << int(board -> color_at[i] == BLACK ? 6 : 0) << endl;
			return false;
		}
	}
	return true;
}

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
	UndoData _undo_data;

	Init();
	POS p;
	UNDO undo_data;

	double first_duration = 0.0;
	double old_duration = 0.0;
	double new_duration = 0.0;
	double sungorus_duration = 0.0;

	for (int game_idx = 0; game_idx < (int) 4e4; game_idx++) {
		SetPosition( & p, START_POS);
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

		// sungorus 
		initial_time = std::chrono::high_resolution_clock::now();
		for (int move_idx = 0; move_idx < moves.size(); move_idx++) {
			DoMove(&p, StrToMove(&p, const_cast<char*>(move_to_str(moves[move_idx]).c_str())), &undo_data);
		}
		end_time = std::chrono::high_resolution_clock::now();
		duration = end_time - initial_time;
		sungorus_duration += duration.count();

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
	printf("Total duration for sungorus	is: %lf\n", sungorus_duration);
}

void test_move_valid_speed() {
	std::string rng_seed_str = "Dratini";
	std::seed_seq _seed(rng_seed_str.begin(), rng_seed_str.end());
	auto rng = std::default_random_engine { _seed };
	std::uniform_int_distribution < uint64_t > dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
	std::chrono::time_point < std::chrono::high_resolution_clock > initial_time, end_time;
	std::vector < Move > moves;
	Board board;
	UndoData _undo_data;
	std::vector<Move> raw_moves, valid_moves;
	Init();
	POS p;
	UNDO undo_data;
	int move_picked;
	Move move;

	double old_mar_duration = 0.0;
	double new_mar_duration = 0.0;
	double new_mar_and_mm_duration = 0.0;
	double movevalid_duration = 0.0;
	double fast_duration = 0.0;
	double old_mar_control_duration = 0.0;
	double sungorus_mar_duration = 0.0;

	int n_games = 5000, game_repetitions = 20;
	long long fingerprint = 0, prev_fingerprint, fingerprint_diff;
	uint64_t prev_king_attackers;

	for(int game_idx = 0; game_idx < n_games; game_idx++) {
		SetPosition(&p, START_POS);
		board = Board();

		for(int move_idx = 0; move_idx < 200; move_idx++) {
			raw_moves.clear();
			valid_moves.clear();

			if(board.king_attackers) {
				generate_evasions(raw_moves, &board);
			} else {
				generate_captures(raw_moves, &board);
				generate_quiet(raw_moves, &board);
			}

			std::vector<int> s_moves;
			for(int i = 0; i < raw_moves.size(); i++) {
				s_moves.push_back(StrToMove(&p, const_cast<char*>(move_to_str(raw_moves[i]).c_str())));	
			}
			assert(s_moves.size() == raw_moves.size());

            for(int j = 0; j < game_repetitions; j++) {
				prev_fingerprint = fingerprint;

                // Old MAR 
				prev_king_attackers = board.king_attackers;
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                    board.make_move(raw_moves[i], _undo_data);
					fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
                    board.take_back(_undo_data);
                }
                old_mar_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				fingerprint_diff = fingerprint - prev_fingerprint;
				prev_fingerprint = fingerprint;

                // New MAR 
				prev_king_attackers = board.king_attackers;
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                	board.make_move(raw_moves[i], _undo_data);
					fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
                    board.new_take_back(_undo_data);
					board.king_attackers = prev_king_attackers;
                }
                new_mar_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == fingerprint - prev_fingerprint);
				prev_fingerprint = fingerprint;

                // New MAR 
				prev_king_attackers = board.king_attackers;
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                	board.new_make_move(raw_moves[i], _undo_data);
					fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
                    board.new_take_back(_undo_data);
					board.king_attackers = prev_king_attackers;
                }
                new_mar_and_mm_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == fingerprint - prev_fingerprint);
				prev_fingerprint = fingerprint;

                // Move valid 
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
					if(board.move_valid(raw_moves[i])) {
						fingerprint++;
						board.make_move(raw_moves[i], _undo_data);
						board.take_back(_undo_data);
					}
                }
                movevalid_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == fingerprint - prev_fingerprint);
				prev_fingerprint = fingerprint;

				// Fast
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                    if(board.fast_move_valid(raw_moves[i])) {
						fingerprint++;
                        valid_moves.push_back(raw_moves[i]);
						board.new_make_move(raw_moves[i], _undo_data);
						board.new_take_back(_undo_data);
					}
                }
                fast_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				fingerprint_diff = fingerprint - prev_fingerprint;
				prev_fingerprint = fingerprint;

                // Dratini Old MAR (control)
				prev_king_attackers = board.king_attackers;
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < raw_moves.size(); i++) {
                    board.make_move(raw_moves[i], _undo_data);
					fingerprint += !board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side);
                    board.take_back(_undo_data);
                }
                old_mar_control_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == fingerprint - prev_fingerprint);
				prev_fingerprint = fingerprint;

                // Sungorus MAR 
                initial_time = std::chrono::high_resolution_clock::now();
                for(int i = 0; i < s_moves.size(); i++) {
                    DoMove(&p, s_moves[i], &undo_data);
					if(!Attacked(&p, p.king_sq[Opp(p.side)], p.side)) fingerprint++;
                    UndoMove(&p, s_moves[i], &undo_data);
                }
                sungorus_mar_duration += (std::chrono::high_resolution_clock::now() - initial_time).count();
				assert(fingerprint_diff == (fingerprint - prev_fingerprint));
				prev_fingerprint = fingerprint;
            }

			// we pick a random move and apply it to all the boards
			if(valid_moves.empty()) break;
			move_picked = dist(rng) % int(valid_moves.size());
			board.make_move(valid_moves[move_picked], _undo_data);
			DoMove(&p, StrToMove(&p, const_cast<char*>(move_to_str(valid_moves[move_picked]).c_str())), &undo_data);
		}
	}

	n_games *= game_repetitions * 2 * 100;

	cout << "Per game duration:" << endl;
	cout << "Old MAR   " << std::setw(12) << int(old_mar_duration / double(n_games)) << endl; 
	cout << "New MAR   " << std::setw(12) << int(new_mar_duration / double(n_games)) << endl; 
	cout << "New MARMM " << std::setw(12) << int(new_mar_and_mm_duration / double(n_games)) << endl; 
	cout << "M valid   " << std::setw(12) << int(movevalid_duration / double(n_games)) << endl; 
	cout << "Fast      " << std::setw(12) << int(fast_duration / double(n_games)) << endl; 
	cout << "OLD MAR C " << std::setw(12) << int(old_mar_control_duration / double(n_games)) << endl; 
	cout << "S MAR     " << std::setw(12) << int(sungorus_mar_duration / double(n_games)) << endl; 
	cout << "Fingerprint " << fingerprint << endl;
	cout << "Fingerprint should be " << 3681050660 << endl;
}

// void test_see_speed() {
// 	std::string rng_seed_str = "Dratini";
// 	std::seed_seq _seed(rng_seed_str.begin(), rng_seed_str.end());
// 	auto rng = std::default_random_engine {
// 		_seed
// 	};
// 	std::uniform_int_distribution < uint64_t > dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
// 	std::chrono::time_point < std::chrono::high_resolution_clock > initial_time, end_time;
// 	std::vector<Move> raw_moves, valid_moves;
// 	Board board;
// 	UndoData _undo_data;
// 	Init();
// 	POS p;
// 	UNDO undo_data;
// 	int score, s_score, move_picked;
// 	Move move;
// 	double dratini_see = 0.0;
// 	double sungorus_see = 0.0;

// 	// constants
// 	int n_games = (int)5e3;
//     int game_repetitions = 50;

// 	for(int game_idx = 0; game_idx < n_games; game_idx++) {
// 		SetPosition(&p, START_POS);
// 		board = Board();

// 		for(int move_idx = 0; move_idx < 200; move_idx++) {
// 			raw_moves.clear();
// 			valid_moves.clear();

// 			if(board.king_attackers) {
// 				generate_evasions(raw_moves, &board);
// 			} else {
// 				generate_captures(raw_moves, &board);
// 				// generate_quiet(raw_moves, &board);
// 			}	

// 			for(int i = 0; i < raw_moves.size(); i++) {
// 				if(board.fast_move_valid(raw_moves[i])) {
// 					valid_moves.push_back(raw_moves[i]);
// 				}
// 			}

// 			score = s_score = 0; 

// 			initial_time = std::chrono::high_resolution_clock::now();
//             for(int j = 0; j < game_repetitions; j++) {
// 				for(int i = 0; i < raw_moves.size(); i++) if(get_flag(raw_moves[i]) == CAPTURE_MOVE) { 
// 					score += board.fast_see(raw_moves[i]);	
// 				}	
// 			}
// 			dratini_see += (std::chrono::high_resolution_clock::now() - initial_time).count();

// 			initial_time = std::chrono::high_resolution_clock::now();
//             for(int j = 0; j < game_repetitions; j++) {
// 				for(int i = 0; i < raw_moves.size(); i++) if(get_flag(raw_moves[i]) == CAPTURE_MOVE) { 
// 					s_score += Swap(&p, get_from(raw_moves[i]), get_to(raw_moves[i]));	
// 				}	
// 			}
// 			sungorus_see += (std::chrono::high_resolution_clock::now() - initial_time).count(); 

// 			initial_time = std::chrono::high_resolution_clock::now();
//             for(int j = 0; j < game_repetitions; j++) {
// 				for(int i = 0; i < raw_moves.size(); i++) if(get_flag(raw_moves[i]) == CAPTURE_MOVE) { 
// 					score += board.fast_see(raw_moves[i]);	
// 				}	
// 			}
// 			dratini_see += (std::chrono::high_resolution_clock::now() - initial_time).count();

// 			initial_time = std::chrono::high_resolution_clock::now();
//             for(int j = 0; j < game_repetitions; j++) {
// 				for(int i = 0; i < raw_moves.size(); i++) if(get_flag(raw_moves[i]) == CAPTURE_MOVE) { 
// 					s_score += Swap(&p, get_from(raw_moves[i]), get_to(raw_moves[i]));	
// 				}	
// 			}
// 			sungorus_see += (std::chrono::high_resolution_clock::now() - initial_time).count(); 

// 			// we pick a random move and apply it to all the boards
// 			if(valid_moves.empty()) break;
// 			move_picked = dist(rng) % int(valid_moves.size());
// 			move = valid_moves[move_picked];
// 			board.make_move(valid_moves[move_picked], _undo_data);
// 			DoMove(&p, StrToMove(&p, const_cast<char*>(move_to_str(move).c_str())), &undo_data);
// 		}
// 	}

// 	n_games *= game_repetitions * 2;

// 	cout << "Per game duration:" << endl;
// 	cout << "Dratini    " << std::setw(10) << int(dratini_see / double(n_games)) / 100 << endl; 
// 	cout << "Sungorus   " << std::setw(10) << int(sungorus_see / double(n_games)) / 100 << endl; 
// }