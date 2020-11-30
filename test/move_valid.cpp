#include "catch.h"
#include "../src/defs.h"
#include "../src/board.h"


TEST_CASE("Test the move validity checking function") {
    Position position;

    REQUIRE(position.move_valid(0, 2) == false);
    REQUIRE(position.move_valid(7, 8) == false);
}