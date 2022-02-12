#include <iostream>
#include <vector>
#include <math.h>
#include "defs.h"
#include "piles.h"
#include <chrono>

using namespace std;

void print(vector<vector<bool>> piles, vector<int> remaining);
int sum_pile(vector<vector<bool>> piles);

int main() {
    int n_piles = 8;
    int n_cubes = 47;

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    auto piles = init_distribution( n_piles,  n_cubes);
    auto remaining = init_remaining(piles, n_cubes);
    auto pos = init_pos(piles);

    auto start = chrono::high_resolution_clock::now();
    auto placed = place(piles, remaining, pos, n_cubes);
    auto stop = chrono::high_resolution_clock::now();
    auto duration = duration_cast<chrono::milliseconds>(stop-start);
    std::cout<<"Completed in "<<duration.count()<<" milliseconds"<<endl;

    print(placed, n_cubes);
}
