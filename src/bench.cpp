#include <string>
#include <iostream>
#include <string.h>
#include <cassert>
#include "engine.h"
#include "defs.h"
#include "board.h"
#include "search.h"
#include "tt.h"

void bench() {
    cout << "mask_sq finally improved" << endl;

    tt.allocate(16);
    
    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    engine.reset();
    engine.max_depth = 12;
    long long total_nodes = 0;
    float total_time = 0.0;

    for(int i = 0; strcmp(Benchmarks[i], ""); i++) {
        std::string line(Benchmarks[i]);
        engine.board = Board(line);
        think(engine);

        printf("Bench #%2d score: %5d, bestmove: %s, ponder: %s, nodes: %7d, nps: %7dK, elapsed: %8dms\n",
                i + 1, engine.score, move_to_str(engine.best_move).c_str(), move_to_str(engine.ponder_move).c_str(), engine.nodes, int(float(engine.nodes) / engine.search_time), engine.search_time); 
        printf("TT percentage: %d percent, totally tried save %d, totally replaced %d, total saved %d\n",
               tt.how_full(), tt.total_tried_save, tt.totally_replaced, tt.total_saved);

        tt.clear();
        total_nodes += engine.nodes;
        total_time += engine.search_time;
    }
    
    printf("Total nps is: %dK\n", int(float(total_nodes) / total_time));
}
