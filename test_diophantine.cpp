#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "diophantine.h"
#include <iostream>
#include "catch.hh"
#include "piles.h"

using namespace std;
auto f = Filter("/Users/rvilim/repos/piles/cubes_8", 60);

TEST_CASE( "Check exact match 9^3", "[diophantane_match]" ) {
    // 1^3 + 6^3 + 8^3 = 9^3, this looks for this mask exactly, with no extra bits set
    // in either the mask or the filters
    REQUIRE (f.search(9,{1,6,8})==true);
}

TEST_CASE( "Check no match 9^3", "[diophantane_match]" ) {
    // 1,6,8,9
    REQUIRE (f.search(9, {1, 7, 8}) ==false);
}

TEST_CASE( "extras (at the start) for 25^3", "[diophantane_match]" ) {
    // 4,17,22|25
    REQUIRE (f.search(25, {1, 4, 15, 17, 18, 22}) == true);
}

TEST_CASE( "extras (in the middle) for 25^3", "[diophantane_match]" ) {
    // 4,17,22|25
    REQUIRE (f.search(25, {4, 15, 17, 18, 22}) == true);
}

TEST_CASE( "No match for 25^3 (extras > 25^3)", "[diophantane_match]" ) {
    // 4,17,22|25
    REQUIRE ( f.search(25, {15, 17, 18, 22, 38}) == false);

}

TEST_CASE( "Match for 25^3 (extras > 25^3)", "[diophantane_match]" ) {
    REQUIRE (f.search(25, {4, 15, 17, 18, 22, 38}) == true);
}
