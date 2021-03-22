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

int main() {
//
// 8   . . . k . . . .
// 7   . . . . . . . .
// 6   . . . . . . . .
// 5   . . . . Q . . .
// 4   . . . . . . . .
// 3   . . . . . . . .
// 2   . . . . . . . .
// 1   . K . . . . . .
//
//     a b c d e f g h

    Board board = Board("8/3k4/8/8/4Q3/8/8/1K6 w - - 0 1");
    // Board board = Board("3k4/8/8/4Q3/8/8/8/1K6 b - - 0 1");    
    cout << "Eval is: " << eval_tscp(board) << endl;
    board.print_board();
}