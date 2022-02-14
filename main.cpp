#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>

using namespace std;
vector<BlockingConcurrentQueue<vector<vector<bool>>>> queues;

int main() {
    const int n_cubes=47;
    const int n_piles=8;

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    int target = sums[n_cubes-1]/n_piles;

    vector<bool> pile(n_cubes, false);
    vector<bool> disallowed(n_cubes, false);

    auto remaining = sums[n_cubes-1];

    vector<vector<bool>> history;

    for(int i=0;i<n_piles;i++){
        queues.emplace_back();
    }

    vector<thread> pile_threads;
    auto start = chrono::high_resolution_clock::now();

    pile_threads.emplace_back(start_source, target, n_piles, n_cubes);

    for(int i=0; i<=n_piles-2;i++){
        pile_threads.emplace_back(start_thread,target, n_piles, n_cubes, i, i+1);
        pile_threads.emplace_back(start_thread,target, n_piles, n_cubes, i, i+1);
        pile_threads.emplace_back(start_thread,target, n_piles, n_cubes, i, i+1);

    }

    pile_threads.emplace_back(start_final, n_piles);

    auto m = std::thread(monitor);

    for(auto & pile_thread : pile_threads){
        pile_thread.join();
    }
    m.join();

    auto stop = chrono::high_resolution_clock::now();
    auto duration = duration_cast<chrono::milliseconds>(stop-start);

    cout<<endl<<"Completed in "<<duration.count()<<" milliseconds"<<endl;

}
