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

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    auto piles = init_distribution();
    auto remaining = init_remaining(piles);
    auto pos = init_pos(piles);

    for(int i=0;i<n_piles;i++){
        cout<<remaining[i]<<endl;

    }

    bitset<n_piles*n_cubes> piles_flat;
    for(int i=0;i<n_piles;i++){
        for(int c=0; c<n_cubes;c++){
            piles_flat.set(i*n_cubes+c, piles[i][c]);
        }
    }

    auto start = chrono::high_resolution_clock::now();
    place(piles_flat, remaining, pos);
    auto stop = chrono::high_resolution_clock::now();
    auto duration = duration_cast<chrono::milliseconds>(stop-start);
    std::cout<<"Completed in "<<duration.count()<<" milliseconds"<<endl;

}
