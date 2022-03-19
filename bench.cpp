#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>
#include <cmath>

using namespace std;
vector<BlockingConcurrentQueue<vector<int>>> queues;

int n_cubes=47;
int n_piles=8;

int run();
double mean(vector<int> &timing);
double stddev(vector<int> &timing);

int main() {
    const unsigned int N = 100;

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
    auto assigned_remaining = init_remaining(assigned_piles);


    vector<int> pile(n_cubes, false);
    vector<int> disallowed(n_cubes, false);
    vector<short> history;
    vector<thread> pile_threads;

    auto start_pos = init_pos(assigned_piles);


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
}