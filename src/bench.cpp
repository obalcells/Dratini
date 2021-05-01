#include <string>
#include <iostream>
#include "engine.h"
#include "defs.h"
#include "board.h"
#include "search.h"
#include "tt.h"

void bench() {
	cerr << "B" << endl;
    tt.allocate(64);
    
    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    std::cout << MAGENTA_COLOR << "First time with engine" << RESET_COLOR << endl; 
    engine.reset();
    cerr << RED_COLOR << "Done constructing the empty board brrr" << RESET_COLOR << endl;
    engine.max_depth = 8;
    long long total_nodes = 0;
    float total_time = 0.0;
	assert(false);

    for(int i = 0; strcmp(Benchmarks[i], ""); i++) {
        std::string line(Benchmarks[i]);
        cerr << RED_COLOR << "Before board construction" << RESET_COLOR << endl;
        assert(false);
        engine.board = Board(line);
        cerr << "The Board was constructed" << endl;
        // assert(false);
        think(engine);

        printf("Bench #%2d score: %5d, bestmove: %s, ponder: %s, nodes: %7d, nps: %7dK, elapsed: %8f\n",
                i + 1, engine.score, move_to_str(engine.best_move).c_str(), move_to_str(engine.ponder_move).c_str(), engine.nodes, int(float(engine.nodes) / engine.search_time), engine.search_time); 

        tt.clear();
        total_nodes += engine.nodes;
        total_time += engine.search_time;
    }
    
    printf("Total nps is: %dK\n", int(float(total_nodes) / total_time));
}
