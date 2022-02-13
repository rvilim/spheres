#include <iostream>
#include <vector>
#include <math.h>
#include <array>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <bitset>

using namespace std;

int main() {
    const int n_cubes=23;
    const int n_piles=3;

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    int target = sums[n_cubes-1]/n_piles;

    vector<bool> pile(n_cubes, false);
    vector<bool> disallowed(n_cubes, false);
    auto remaining = cubes[n_cubes-1];
    make_pile(target, remaining,n_cubes-1, pile, disallowed, n_piles, n_cubes);

}
