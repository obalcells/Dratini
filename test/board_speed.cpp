#include <sys/timeb.h>
#include <iostream>
#include <random>
#include <vector>
#include "../src/gen.h"
#include "../src/board.h"
#include "sungorus_board.h"

int StrToMove(POS *p, char *move_str)
{
  int from, to, type;

  from = Sq(move_str[0] - 'a', move_str[1] - '1');
  to = Sq(move_str[2] - 'a', move_str[3] - '1');
  type = NORMAL;
  if (TpOnSq(p, from) == K && Abs(to - from) == 2)
    type = CASTLE;
  else if (TpOnSq(p, from) == P) {
    if (to == p->ep_sq) 
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

bool check_same(POS* p, Board* board) {
    for(int i = 0; i < 64; i++) {
        if(p->pc[i] == NO_PC && board->piece_at[i] != EMPTY)
            return false;
        if(p->pc[i] != NO_PC && board->piece_at[i] == EMPTY)
            return false;
        if(p->pc[i] == NO_PC && board->piece_at[i] == EMPTY)
            continue;
        int p1 = (p->pc[i] >> 1) + ((p->pc[i] & 1) ? 6 : 0);
        int p2 = board->piece_at[i] + (board->color_at[i] == BLACK ? 6 : 0);
        if(p1 != p2) {
            cerr << "At position " << i << " pieces are not the same" << endl;
            cerr << (int)p->pc[i] << " " << (int)board->piece_at[i] << " " << int(board->color_at[i] == BLACK ? 6 : 0) << endl;
            return false;
        }
    }
    return true;
}
        
void test_board_speed() {
    std::string rng_seed_str = "Dratini";
    std::seed_seq _seed (rng_seed_str.begin(), rng_seed_str.end());
    auto rng = std::default_random_engine { _seed };
    std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 56)), std::llround(std::pow(2, 62)));
    std::chrono::time_point<std::chrono::high_resolution_clock> initial_time, end_time;
    std::chrono::duration<double, std::milli> duration;
    std::vector<Move> moves;
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

    for(int game_idx = 0; game_idx < (int)4e4; game_idx++) {
        SetPosition(&p, START_POS);
        first_board = board = new_board = Board();
        moves.clear();

        // we generate the list of moves
        for(int move_idx = 0; move_idx < 200; move_idx++) {
            std::vector<Move> generated_moves;
            generate_moves(generated_moves, &first_board, false);
            if(generated_moves.empty() || first_board.fifty_move_ply >= 50)
              break;
            int move_picked = dist(rng) % int(generated_moves.size()); 
            Move move = generated_moves[move_picked];
            assert(first_board.move_valid(move));
            moves.push_back(move);
            first_board.make_move(move);
            new_board.new_make_move(move, _undo_data);
            new_board.update_classic_settings();
            if(!first_board.same(new_board)) {
              cout << "Boards are different after move " << move_to_str(move) << endl;    
              cout << "Board is:" << endl;
              first_board.print_board();
              assert(false);
            }
        }

        // first board (for some reason the board that runs first takes more time)
        first_board = Board();
        initial_time = std::chrono::high_resolution_clock::now();
        for(int move_idx = 0; move_idx < moves.size(); move_idx++) {
            first_board.make_move(moves[move_idx]);
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = end_time - initial_time;
        first_duration += duration.count();

        // base
        initial_time = std::chrono::high_resolution_clock::now();
        for(int move_idx = 0; move_idx < moves.size(); move_idx++) {
            board.make_move(moves[move_idx]);
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = end_time - initial_time;
        old_duration += duration.count();

        // dev
        new_board = Board();
        initial_time = std::chrono::high_resolution_clock::now();
        for(int move_idx = 0; move_idx < moves.size(); move_idx++) {
            new_board.new_make_move(moves[move_idx], _undo_data);
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = end_time - initial_time;
        new_duration += duration.count();

        // sungorus 
        initial_time = std::chrono::high_resolution_clock::now();
        for(int move_idx = 0; move_idx < moves.size(); move_idx++) {
            DoMove(&p, StrToMove(&p, const_cast<char *>(move_to_str(moves[move_idx]).c_str())), &undo_data);
        }
        end_time = std::chrono::high_resolution_clock::now();
        duration = end_time - initial_time;
        sungorus_duration += duration.count();
    }

            // initial_time = std::chrono::high_resolution_clock::now();
            // board.make_move(move);
            // end_time = std::chrono::high_resolution_clock::now();
            // duration = end_time - initial_time;
            // old_duration += duration.count();
            // board.error_check();

            // if(!board.same(new_board)) {
            //   cout << "Boards are not the same after move " << move_to_str(move) << endl;
            //   cout << "Traditional board is:" << endl;
            //   board.print_board();
            //   cout << "Board with new movemake is:" << endl;
            //   new_board.print_board();
            // }

            // char* str_move = const_cast<char *>(move_to_str(move).c_str());
            // int sungorus_move = StrToMove(&p, str_move);
            // initial_time = std::chrono::high_resolution_clock::now();

            // DoMove(&p, sungorus_move, &undo_data, &board);
            // end_time = std::chrono::high_resolution_clock::now();
            // duration = end_time - initial_time;
            // sungorus_duration += duration.count();

            // if(!check_same(&p, &board)) {
            //     cerr << "Boards are no longer the same after move " << move_to_str(move) << endl;
            //     assert(false);
            // }
    //     }
    // }

    printf("Total duration for FIRST board is: %lf\n", first_duration);
    printf("Total duration for OLD   board is: %lf\n", old_duration);
    printf("Total duration for NEW   board is: %lf\n", new_duration);
    printf("Total duration for sungorus    is: %lf\n", sungorus_duration);
}
