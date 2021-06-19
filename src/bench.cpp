#include <string>
#include <iostream>
#include <string.h>
#include <cassert>
#include "engine.h"
#include "defs.h"
#include "board.h"
#include "search.h"
#include "new_search.h"
#include "tt.h"
#include "nnue.h"
#include "sungorus_eval.h"

void bench() {
    tt.allocate(16);
    
    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    engine.reset();

    // // The Incremental Test!
    // UndoData undo_data(engine.board.king_attackers);
    // engine.board = Board();
    // cerr << engine.board.acc_stack[0].has_been_computed << endl;
    // nnue_eval(engine.board);
    // cerr << engine.board.acc_stack[0].has_been_computed << endl;
    // engine.board.new_make_move(Move(A2, A3, QUIET_MOVE), undo_data);
    // cerr << "Sz of stack is " << engine.board.acc_stack_size << endl;
    // cerr << engine.board.acc_stack[0].has_been_computed << endl;
    // cerr << engine.board.dp_stack[0].no_king << endl;
    // nnue_eval(engine.board);
    // return;

    engine.max_depth = 12;
    long long total_nodes = 0;
    float total_time = 0.0;

    for(int i = 0; strcmp(Benchmarks[i], ""); i++) {
        std::string line(Benchmarks[i]);
        engine.board = Board(line);
        think(engine);
        // printf("Bench #%2d score: %5d, bestmove: %s, ponder: %s, nodes: %7d, nps: %7dK, elapsed: %8dms\n",
        //         i + 1, engine.score, move_to_str(engine.best_move).c_str(), move_to_str(engine.ponder_move).c_str(), engine.nodes, int(float(engine.nodes) / engine.search_time), engine.search_time); 
        tt.clear();
        total_nodes += engine.nodes;
        total_time += engine.search_time;
    }
    
    printf("Total nps is: %dK\n", int(float(total_nodes) / total_time));
    printf("Total time is %d\n", int(total_time));
}
