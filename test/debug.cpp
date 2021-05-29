#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>
#include "../src/defs.h"
#include "../src/tt.h"
#include "../src/board.h"
#include "../src/gen.h"
#include "../src/engine.h"
#include "sungorus_board.h"
#include "board_speed.h"

TranspositionTable tt;
Engine engine;

static std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> tokens;
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    while ((end = str.find(" ", start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

long long old_perft(Board&, int); 
long long new_perft(Board&, int); 
long long s_perft(POS*, int);

// constants
static const int max_depth = 5;
static const int n_runs = 10;
static long long time_gen = 0;


static std::chrono::time_point<std::chrono::high_resolution_clock> t;

int main() {
    // test_board_speed();
    // test_move_valid_speed();

    Board board = Board();
    Init();
    POS pos;
    SetPosition(&pos, START_POS);
    std::chrono::time_point < std::chrono::high_resolution_clock > initial_time;

    long long duration_1 = 0;
    long long duration_2 = 0;
    long long duration_3 = 0;

    for(int i = 0; i < n_runs; i++) {
        // initial_time = std::chrono::high_resolution_clock::now();
        // old_perft(board, 0);
        // duration_1 += (long long)(std::chrono::high_resolution_clock::now() - initial_time).count();

        initial_time = std::chrono::high_resolution_clock::now();
        new_perft(board, 0);
        duration_2 += (long long)(std::chrono::high_resolution_clock::now() - initial_time).count();

        initial_time = std::chrono::high_resolution_clock::now();
        s_perft(&pos, 0);
        duration_3 += (long long)(std::chrono::high_resolution_clock::now() - initial_time).count();
    } 

    // adjusted rates
    // duration_1 /= (long long)1e3 * n_runs;
    duration_2 /= (long long)1e3 * n_runs;
    // time_gen /= (long long)1e3 * n_runs;
    duration_3 /= (long long)1e3 * n_runs;

    cerr << BLUE_COLOR << "Dratini" << endl << RESET_COLOR;
    cerr << "Total time " << duration_2 << endl;

    cerr << BLUE_COLOR << "Sungorus" << endl << RESET_COLOR;
    cerr << "Total time " << duration_3 << endl;
    // cerr << "Doing gen  " << time_gen << endl;

    // if(duration_2 <= duration_3) cout << GREEN_COLOR << "New Perft duration is " << duration_2 << endl << RESET_COLOR;
    // else                         cout << RED_COLOR << "New Perft duration is " << duration_2 << endl << RESET_COLOR;
    // if(duration_3 < duration_2) cout << GREEN_COLOR << "Sun Perft duration is " << duration_3 << endl << RESET_COLOR;
    // else                         cout << RED_COLOR << "Sun Perft duration is " << duration_3 << endl << RESET_COLOR;

    // Board board = Board("8/8/R6Q/3p4/2k5/1r6/8/4K3 w - - 0 1");
    // board.print_board();

    // Move move = Move(C2, C4, QUIET_MOVE); 

    // cout << (board.move_valid(move) ? "valid" : "not valid");

    // int king_position = lsb(board.bits[WHITE_KING]);
    // cerr << "Position of king is " << king_position << endl;

    // uint64_t attackers = get_attackers(king_position, board.side, &board);

    // cout << "Attackers mask is " << attackers << endl;
    // cout << "Position of first attacker is " << lsb(attackers) << endl;

    // Board board = Board("8/3k4/8/8/4Q3/8/8/1K6 w - - 0 1");
    // // Board board = Board("3k4/8/8/4Q3/8/8/8/1K6 b - - 0 1");    
    // cout << "Eval is: " << eval_tscp(board) << endl;
    // board.print_board();

    return 0;
}

long long s_perft(POS *pos, int depth) {
    int move[128];
	// t = std::chrono::high_resolution_clock::now();
    GenerateCaptures(pos, move);
    int* last = GenerateQuiet(pos, move);
    // time_gen += (long long)(std::chrono::high_resolution_clock::now() - t).count();
    if(depth == max_depth)
        return last - move;  // ?
    UNDO undo_data;
    long long ans = 0;
    for(int* it = move; it < last; it++) {
        DoMove(pos, *it, &undo_data);
        if(Illegal(pos)) {
            UndoMove(pos, *it, &undo_data);
            continue;
        } 
        ans += s_perft(pos, depth + 1);
        UndoMove(pos, *it, &undo_data);
    }
    return ans;
}

long long new_perft(Board& board, int depth) {
    // std::vector<Move> moves;
    // moves.reserve(128);
    int moves[128];
	// t = std::chrono::high_resolution_clock::now();
    new_generate_captures(moves, &board);
    int* last = new_generate_quiet(moves, &board);
    // time_gen += (long long)(std::chrono::high_resolution_clock::now() - t).count();
    if(depth == max_depth)
        return last - moves;
    UndoData undo_data = UndoData(board.king_attackers);
    long long ans = 0;
    for(auto move : moves) {
        board.new_make_move(move, undo_data);
        if(!board.is_attacked(lsb(board.bits[KING + (board.xside ? 6 : 0)]), board.side))
            ans += new_perft(board, depth + 1);
        board.new_take_back(undo_data);
        // if(board.new_fast_move_valid(move)) {
        //     board.new_make_move(move, undo_data);
        //     ans += new_perft(board, depth + 1);
        //     board.new_take_back(undo_data);
        // }
    }
    return ans;
}

long long old_perft(Board& board, int depth) {
    std::vector<Move> moves;
    moves.reserve(128);
    generate_captures(moves, &board);
    generate_quiet(moves, &board);
    if(depth == max_depth)
        return moves.size();
    UndoData undo_data = UndoData(board.king_attackers);
    long long ans = 0;
    for(auto move : moves) {
        if(board.fast_move_valid(move)) {
            board.make_move(move, undo_data);
            ans += old_perft(board, depth + 1);
            board.take_back(undo_data);
        }
    }
    return ans;
}