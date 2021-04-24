#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>

#include "../src/defs.h"
#include "../src/position.h"
#include "../src/eval_tscp.h"
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
    std::string s = "position startpos moves g1f3 a7a6 e2e3 a8a7 f3g5 e7e6 d1h5 g7g6 h5h4 g8f6 f1e2 f8c5 e2d3 b8c6 g5f3 h8f8 h4h3 d7d6 h3h4 c8d7 h4h6 c6b4 a2a4 d7c6 f3h4 b4d3 c2d3 f8g8 h4f3 g6g5 f3g5 f6g4 h6h7 d8g5 h2h4 g5g7 h7g7 g8g7 h1h3 g4e5 e1e2 c6g2 h3h2 g2f3 e2f1 e5g4 h4h5";
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