#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.h"
#include "../src/defs.h"
#include "../src/stats.h"
#include "../src/board.h"
#include "../src/tt.h"

/*
    1) Legality checking works comparing to slow engine 10s
        a) Convert Position Class into 
    2) Generated moves are legal comparing with slow engine 30s
    3) Generated moves whose legality shouldn't have to be checked are legal  
    4) Measure performance using tests
*/

Statistics stats;
TranspositionTable tt;