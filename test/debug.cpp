#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>
#include "../src/defs.h"
#include "../src/tt.h"
#include "../src/board.h"
#include "../src/engine.h"
#include "board_speed.hpee

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
    test_board_speed();
    // std::string s = "position startpos moves d2d3 b8c6 b1c3 e7e5 g1f3 g8f6 e2e4 d7d5 d1e2 d5d4 c3d1 a7a5 c2c3 f8c5 c3d4 c6d4 f3d4 c5d4 c1e3 a5a4 h2h4 c8e6 d1c3 a4a3 e3d4 d8d4 a1b1 a8a6 e2d2 a6c6 f2f3 a3b2 c3e2 d4a7 b1b2 e8g8 e2c3 f8a8 g2g4 a7a5 b2c2 b7b5 f1e2 b5b4 c3d5 c6c2 d5f6 g7f6 d2c2 a5b6 c2d1 a8a2 d3d4 b4b3 e1g1 b3b2 d1c2 b6d4 g1g2 b2b1q c2b1 a2e2 g2h1 d4d2 b1b8 g8g7 b8f8 g7f8 f1a1";
    // // std::string s = "position startpos moves d2d3 b8c6 b1c3 e7e5 g1f3 g8f6 e2e4 d7d5 d1e2 d5d4 c3d1 a7a5 c2c3 f8c5 c3d4 c6d4 f3d4 c5d4 c1e3 a5a4 h2h4 c8e6 d1c3 a4a3 e3d4 d8d4 a1b1 a8a6 e2d2 a6c6 f2f3 a3b2 c3e2 d4a7 b1b2 e8g8 e2c3 f8a8 g2g4 a7a5 b2c2 b7b5 f1e2 b5b4 c3d5 c6c2 d5f6 g7f6 d2c2 a5b6 c2d1 a8a2 d3d4 b4b3 e1g1 b3b2 d1c2 b6d4 g1g2 b2b1q";
    // std::vector<std::string> args = split(s);
    // Board board = Board();

    // for(int i = 3; i < (int)args.size(); i++) {
    //     if(!board.make_move_from_str(args[i])) {
    //         cout << "There was an error making move " << args[i] << endl;
    //     }
    // }
    
    // board.print_board();

    // Board board = Board("8/3k4/8/8/4Q3/8/8/1K6 w - - 0 1");
    // // Board board = Board("3k4/8/8/4Q3/8/8/8/1K6 b - - 0 1");    
    // cout << "Eval is: " << eval_tscp(board) << endl;
    // board.print_board();

    return 0;
}