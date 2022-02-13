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
    vector<vector<bool>> expected_1(12, vector<bool>(63, false));

    for(int i=0; i<=8; i++){
        expected_1[i][63-i-1]=true;
    }
    REQUIRE( init_distribution( 12,  63) == expected_1);

    vector<vector<bool>> expected_2(3, vector<bool>(23, false));
    expected_2[0][22]=true;

    REQUIRE( init_distribution( 3,  23) == expected_2);
}

TEST_CASE( "Check that the initial pos is calculated correctly", "[pos]" ) {
    vector<vector<bool>> piles_1(12, vector<bool>(63, false));

    for(int i=0; i<=8; i++){
        piles_1[i][63-i-1]=true;
    }
    REQUIRE(init_pos(piles_1)==53);

    vector<vector<bool>> piles_2(3, vector<bool>(23, false));
    piles_2[0][22]=true;

    REQUIRE(init_pos(piles_2)==21);

}

TEST_CASE( "Check that init_remaining is working correctly", "[init_remaining]" ) {
    vector<vector<bool>> piles_1(3, vector<bool>(23, false));
    piles_1[0][22]=true;
    vector<int> expected_1 = {25392-23*23*23, 25392, 25392};

    REQUIRE(init_remaining(piles_1, 23)==expected_1);
}


