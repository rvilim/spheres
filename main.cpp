#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>

using namespace std;
vector<BlockingConcurrentQueue<vector<array<int, 60>>>> queues;
vector<array<int, 60>> assigned_piles;
vector<array<int, 60>> assigned_disallowed;
vector<int> assigned_starts;
vector<int> assigned_remaining;
const int max_lookup_bits=24;

int main() {

    const int n_cubes=23;
    const int n_piles=3;

    assigned_piles = init_distribution(n_cubes, n_piles);
    assigned_remaining = init_remaining( n_cubes,  n_piles, assigned_piles);

    print_piles(assigned_piles);

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    array<int, 60> pile{};
    pile.fill(false);

    array<int, 60> disallowed{};
    disallowed.fill(false);

    vector<array<int, 60>> history;

    for(int i=0;i<n_piles;i++){
        queues.emplace_back();
    }
/*
    make_pile(25392, 25392, 22, pile, disallowed, history, 0, 3, 23);

    vector<array<int, 60>> piles;
    if (queues[0].try_dequeue(piles)){
        print_piles(piles);
    }else{
        cout<<"No piles found"<<endl;
    }*/

    vector<thread> pile_threads;
    auto start = chrono::high_resolution_clock::now();

    pile_threads.emplace_back(start_source, assigned_remaining[0], n_piles, n_cubes,  assigned_piles[0]);


    for(int i=0; i<=n_piles-2;i++){
        pile_threads.emplace_back(start_thread,assigned_remaining[i+1], n_piles, n_cubes, i, i+1, assigned_piles[i+1]);
    }

    // auto m = thread(monitor);

    for(auto & pile_thread : pile_threads){
        pile_thread.join();
    }
    // m.join();

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop-start);

     cout<<endl<<"Completed in "<<duration.count()<<" milliseconds"<<endl;

    vector<array<int, 60>> piles;
    if (queues[n_piles-1].try_dequeue(piles)){
        print_piles(piles);
    }else{
        cout<<"No piles found"<<endl;
    }
}
