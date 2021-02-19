#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <string>

#include "../src/defs.h"
#include "../src/position.h"
#include "../src/board.h"
#include "../src/tt.h"
#include "../src/gen.h"
#include "../src/move_picker.h"

TranspositionTable tt;

int main() {
    Position position = Position();

    cerr << "Board is:" << endl;
    position.get_board().print_board();

    std::vector<Move> moves;
    moves.clear();

    generate_quiet(moves, &position.get_board());

    for(int i = 0; i < moves.size(); i++) {
        cerr << move_to_str(moves[i]) << " ";
    }

    cerr << endl;
}

/*

8   r n b q k . . r
7   p p p p . p p p
6   . . . . p n . .
5   . . . . . . . .
4   . P . P P . . .
3   . . . . . . . .
2   P . P . K P P P
1   R N B Q . B N R

    a b c d e f g h

Move count: 4
Fifty move count: 2
Enpassant: None
Castling (WQ, WK, BQ, BK): N N Y Y 
Side: BLACK
e6e5 a7a6 b7b6 c7c6 d7d6 g7g6 h7h6 a7a5 b7b5 c7c5 d7d5 g7g5 h7h5 e8g8 e8e7 e8f8 f6g4 f6d5 f6h5 f6g8 b8a6 b8c6 b4e1 b4d2 b4a3 b4c3 b4a5 b4c5 b4d6 b4e7 b4f8 d8e7 h8f8 h8g8 

*/