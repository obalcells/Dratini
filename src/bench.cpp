#include <string>
#include <iostream>
#include "defs.h"
#include "board.h"
#include "search.h"
#include "tt.h"

void bench() {
    tt.allocate(64);
    
    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    Engine engine;
    engine.max_depth = 8;
    long long total_nodes = 0;
    float total_time = 0.0;

    for(int i = 0; strcmp(Benchmarks[i], ""); i++) {
        std::string line(Benchmarks[i]);
        engine.board = Board(line);
        think(engine);

        printf("Bench #%2d score: %5d, bestmove: %s, ponder: %s, nodes: %7d, nps: %7dK, elapsed: %8f\n",
                i + 1, engine.score, move_to_str(engine.best_move).c_str(), move_to_str(engine.ponder_move).c_str(), engine.nodes, int(float(engine.nodes) / engine.search_time), engine.search_time); 

        tt.clear();
        total_nodes += engine.nodes;
        total_time += engine.search_time;
    }
    
    printf("Total nps is: %dK\n", int(float(total_nodes) / total_time));
}
