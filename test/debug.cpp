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

int main() {
    // test_board_speed();
    test_move_valid_speed();
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