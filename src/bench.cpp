#include "board.h"

void run_benchmark() {

    static const char *benchmarks[] = {
        #include "bench.csv"
        ""
    };

    Board board;
    Thread *threads;
    Limits limits = {0};

    int scores[256];
    double times[256];
    uint64_t nodes[256];
    uint16_t bestMoves[256];
    uint16_t ponderMoves[256];

    double time;
    uint64_t totalNodes = 0ull;

    int depth     = argc > 2 ? atoi(argv[2]) : 13;
    int nthreads  = argc > 3 ? atoi(argv[3]) :  1;
    int megabytes = argc > 4 ? atoi(argv[4]) : 16;

    initTT(megabytes);
    time = getRealTime();
    threads = createThreadPool(nthreads);

    // Initialize a "go depth <x>" search
    limits.multiPV        = 1;
    limits.limitedByDepth = 1;
    limits.depthLimit     = depth;

    for (int i = 0; strcmp(Benchmarks[i], ""); i++) {

        // Perform the search on the position
        limits.start = getRealTime();
        boardFromFEN(&board, Benchmarks[i], 0);
        getBestMove(threads, &board, &limits, &bestMoves[i], &ponderMoves[i]);

        // Stat collection for later printing
        scores[i] = threads->info->values[depth];
        times[i] = getRealTime() - limits.start;
        nodes[i] = nodesSearchedThreadPool(threads);

        clearTT(); // Reset TT between searches
    }

    printf("\n=================================================================================\n");

    for (int i = 0; strcmp(Benchmarks[i], ""); i++) {

        // Convert moves to typical UCI notation
        char bestStr[6], ponderStr[6];
        moveToString(bestMoves[i], bestStr, 0);
        moveToString(ponderMoves[i], ponderStr, 0);

        // Log all collected information for the current position
        printf("Bench [# %2d] %5d cp  Best:%6s  Ponder:%6s %12d nodes %8d nps\n", i + 1, scores[i],
            bestStr, ponderStr, (int)nodes[i], (int)(1000.0f * nodes[i] / (times[i] + 1)));
    }

    printf("=================================================================================\n");

    // Report the overall statistics
    time = getRealTime() - time;
    for (int i = 0; strcmp(Benchmarks[i], ""); i++) totalNodes += nodes[i];
    printf("OVERALL: %53d nodes %8d nps\n", (int)totalNodes, (int)(1000.0f * totalNodes / (time + 1)));

    free(threads);
}

