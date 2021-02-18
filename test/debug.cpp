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
    std::vector<Move> moves;
    generate_moves(moves, &position.get_board());

    for(int i = 0; i < (int)moves.size(); i++) {
        cerr << move_to_str(moves[i]) << " ";
    }
    cerr << endl;
}
