#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <vector>

#include "piles.h"
#include "catch.hh"

using namespace std;

TEST_CASE( "Check sum of pile", "[sum_pile]" ) {

    vector<bool> p = {true,false, true};

    REQUIRE( sum_pile(p) == 28 );
}

TEST_CASE( "Check initial cube placement", "[init_distribution]" ) {
    REQUIRE( init_distribution( 12,  63) == vector<vector<int>>{{250047}, {238328}, {226981}, {216000}, {205379}, {195112}, {185193}, {175616}, {166375}, {}, {}, {}});
    REQUIRE( init_distribution( 3,  23) == vector<vector<int>>{{23*23*23}, {}, {}});
}



