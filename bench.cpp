#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>
#include <cmath>

using namespace std;
vector<BlockingConcurrentQueue<vector<int_fast8_t>>> queues;

int n_cubes=47;
int n_piles=8;

int run();
double mean(vector<int> &timing);
double stddev(vector<int> &timing);

int main() {
    const unsigned int N = 10;

    vector<int> timing;
    for(int i=0;i<N;i++){
        timing.emplace_back(run());
    }
    cout.precision(5);
    cout<<"μ="<<mean(timing)<<", σ="<<stddev(timing);
}

double mean(vector<int> &timing) {
    double u = 0;

    for (int t: timing) u += t;
    u/=timing.size();
    return u;
}

double stddev(vector<int> &timing) {
    double u = mean(timing);
    double std = 0;

    for(int i=0;i<timing.size();i++){
        std+=pow(timing[i]-u,2)/timing.size();
    }
    return sqrt(std);

}

int run(){

    auto assigned_piles = init_distribution();
    vector<int_fast8_t > pile(n_cubes,0);

    auto remaining = calc_remaining(pile,1);
    auto target = sums[n_cubes-1]/n_piles - sum_pile(pile, 1);
    queues.emplace_back();

    auto start_time = chrono::high_resolution_clock::now();

    make_pile(target, remaining, n_cubes-1, 1, pile, queues[0]);


    auto stop_time = chrono::high_resolution_clock::now();
    auto duration = duration_cast<chrono::milliseconds>(stop_time-start_time);

    queues.clear();
    stop=false;

    return duration.count();
/*

    vector<int> pile(n_cubes, false);
    vector<int> disallowed(n_cubes, false);
    vector<short> history;
    vector<thread> pile_threads;

    for(int i=0;i<n_piles;i++){
        queues.emplace_back();
    }

    auto start_time = chrono::high_resolution_clock::now();

    pile_threads.emplace_back(start_source, assigned_remaining[0],  assigned_piles[0]);

    for(int i=0; i<=n_piles-2;i++){
        pile_threads.emplace_back(start_thread,assigned_remaining[i+1], i, i+1, assigned_piles[i+1], start_pos);
    }

    for(auto & pile_thread : pile_threads){
        pile_thread.join();
    }

    auto stop_time = chrono::high_resolution_clock::now();
    auto duration = duration_cast<chrono::milliseconds>(stop_time-start_time);

    queues.clear();
    stop=false;

    return duration.count();
    */
}