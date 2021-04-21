#include "board.h"
#include "defs.h"

TranspositionTable tt;

void bench() {
    tt.init();
    
    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    Engine engine;
    engine.max_depth = 8;

    for(int i = 0; strcmp(Benchmark[i], ""); i++) {
        engine.position.set_from_fen(line);
        search(engine);

        printf("Bench #%d score: %5d, bestmove: %s, ponder: %s, nodes: %d, nps: %f, elapsed: %f",
                i + 1, engine.score, engine.best_move, engine.ponder_move, engine.nodes, engine.nodes /  

        tt.clear();
        total_nodes += engine.nodes;
        total_time += engine.search_time;
    }
    
    printf("Total nps is: %f", float(total_nodes) / total_time);
        
    return 0;
}
