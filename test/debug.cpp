#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>

#include "../src/defs.h"
#include "../src/position.h"
#include "../src/tt.h"
TranspositionTable tt;

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
    // std::string s = "position startpos moves g1f3 a7a6 e2e3 a8a7 f3g5 e7e6 d1h5 g7g6 h5h4 g8f6 f1e2 f8c5 e2d3 b8c6 g5f3 h8f8 h4h3 d7d6 h3h4 c8d7 h4h6 c6b4 a2a4 d7c6 f3h4 b4d3 c2d3 f8g8 h4f3 g6g5 f3g5 f6g4 h6h7 d8g5 h2h4 g5g7 h7g7 g8g7 h1h3 g4e5 e1e2 c6g2 h3h2 g2f3 e2f1 e5g4 h4h5";
    std::string s = "position startpos moves d2d4 e7e6 b1c3 g8f6 g1f3 f8b4 e2e3 b4c3 b2c3 f6d5 c3c4 d5f6 f1d3 d7d6 e1g1 b8c6 a1b1 f6d7 g1h1 h7h6 d4d5 c6b8 e3e4 d8e7 c1e3 e6e5 d1e1 b8a6 f1g1 d7c5 g2g4 e7f6 e1e2 f6g6 f3h4 g6h7 h4f5 c8f5 e4f5 b7b6 f2f3 c5d3 c2d3 e8d8 a2a3 a6b8 g1f1 h6h5 a3a4 b8d7 a4a5 b6a5 b1a1 h5g4 f3g4 h7h3 a1a5 a7a6 f1a1 d7b8 e3g5 f7f6 g5e3 h8h7 a5a2 h3g3 a1g1 g3h3 a2b2 b8d7 b2b1 a8b8 b1b8 d7b8 e2f2 d8c8 g1g3 h3h4 f2g2 c8d8 e3a7 b8d7 a7f2 h4g5 f2e3 g5h4 h2h3 h7h8 h1h2 d8c8 g2b2 h4h7 b2a3 c8b7 a3b4 b7a8 b4a5 a8b7 a5a4 d7b8 a4b4 b7c8 e3a7 b8d7 b4a4 c8d8 a4a6 h7g8 a6a5 g8f7 a7e3 f7g8 a5a7 d8c8 a7a8 d7b8 h2g1 c8d7 g3f3 g8c8 a8a4 d7d8 a4b5 d8e7 b5a5 h8h7 g1h1 e7f7 a5a8 f7e7 a8a2 c8h8 a2g2 h8e8 f3f1 b8a6 h1h2 e8a8 f1a1 a8b7 a1a2 b7a8 a2a5 e7d7 g4g5";
    // s += ...
    std::vector<std::string> args = split(s);
    Board board = Board();

    for(int i = 3; i < (int)args.size(); i++) {
        if(!board.make_move_from_str(args[i])) {
            cout << "There was an error making move " << args[i] << endl;
        }
    }
    
    board.print_board();

    // Board board = Board("8/3k4/8/8/4Q3/8/8/1K6 w - - 0 1");
    // // Board board = Board("3k4/8/8/4Q3/8/8/8/1K6 b - - 0 1");    
    // cout << "Eval is: " << eval_tscp(board) << endl;
    // board.print_board();

    return 0;
}