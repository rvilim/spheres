#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <string>
#include <cstdlib>

using namespace std;
//namespace po = boost::program_options;

vector<BlockingConcurrentQueue<vector<int_fast8_t>>> queues;

int n_cubes=-1;
int n_piles=-1;
int start_pile=-1;
int end_pile=-1;
//vector<vector<boost::dynamic_bitset<>>> filters;
vector<vector<vector<int_fast8_t>>> filters;

int read_cmd(int argc, char* argv[]);

int read_cmd(int argc, char* argv[]){
    char* p;
    n_piles = strtol(argv[1], &p, 10);
    if (*p != '\0') cout<<"Error";

    n_cubes = strtol(argv[2], &p, 10);
    if (*p != '\0') cout<<"Error";

    start_pile = strtol(argv[3], &p, 10);
    if (*p != '\0') cout<<"Error";

    end_pile = strtol(argv[4], &p, 10);
    if (*p != '\0') cout<<"Error";

}

int main(int argc,char *argv[]) {
    vector<thread> pile_threads;

    read_cmd(argc, argv);

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    filters = read_filters("/Users/rvilim/repos/piles/cubes_8", n_cubes);

//    vector<int_fast8_t> pile = {1, 1, 0, 0, 0, 1, 0, 1, 0};
//    cout<<filter_diophantine(pile, 1)<<endl;
//    return 0;

    vector<thread> threads;
    std::thread monitor_thread(monitor);

    for(int pile_num=0;pile_num<(2+end_pile-start_pile);pile_num++){
        queues.emplace_back();
    }

    for (int pile_num=start_pile; pile_num<=end_pile; pile_num++) {
        if (pile_num == 1) {
            auto assigned = init_distribution();
            queues[0].enqueue(assigned);
        }else if (pile_num==start_pile) {
            auto filename = to_string(n_piles)+"_"+to_string(n_cubes)+"_"+to_string(pile_num-1);
            threads.emplace_back(read_pile, filename, pile_num, 0);
        }

        if (pile_num==n_piles) {
            // We are going until the final pile, terminate as soon as we get an answer and log to stdout
            auto filename = to_string(n_piles)+"_"+to_string(n_cubes)+"_"+to_string(pile_num)+".txt";
            threads.emplace_back(write_pile, pile_num, pile_num, filename, true);
        }else if (pile_num==end_pile) {
            auto filename = to_string(n_piles)+"_"+to_string(n_cubes)+"_"+to_string(pile_num);
            threads.emplace_back(write_pile, pile_num, pile_num, filename, false);

        }

//        threads.emplace_back(solve, pile_num-1, pile_num, pile_num);
        threads.emplace_back(solve, pile_num-1, pile_num, pile_num);

        threads.emplace_back( monitor);
    }

    for (auto & thread : threads) {
        thread.join();
    }
    monitor_thread.join();


}








//        queues.emplace_back(); // Make a source queue
//        auto source_queue = &queues[queues.size()-1];
//
//        if (pile_num==1){ // If this is pile 1 we need to enqueue from the initial distribution
//            auto assigned = init_distribution();
//            source_queue->enqueue(assigned);
//        }if(pile_num==start_pile){ // If this is the first pile (but not pile 1) we enqueue from a file
//            // start enqueueing from file
//        }else{ // If this is some other pile we are using an internal queue
//
//        }
//
//


/*
    if (argc==5){
        auto assigned = init_distribution();
        auto remaining = calc_remaining(assigned,1);
        auto target = sums[n_cubes-1]/n_piles - sum_pile(assigned, 1);

        std::thread write_thread(write, out_file, 1);

        make_pile(target, remaining, n_cubes-1, 1, assigned);
        stop=true;

        write_thread.join();
        cout<<sums[n_cubes-1]/n_piles<<" "<<remaining<<endl;
    }else{
        std::thread read_thread(read, in_file, pile_num );
        std::thread write_thread(write, out_file, pile_num);
        std::thread monitor_thread(monitor);

        vector<thread> pile_threads;

        for(int i=0; i<work_threads;i++){
            pile_threads.emplace_back(solve, pile_num);
        }

        for(auto & pile_thread : pile_threads){
            pile_thread.join();
        }

        read_thread.join();
        write_thread.join();
        monitor_thread.join();
        stop=true;

//        write_thread.join();
//        read_thread.join();


    }

*/


/*

   std::istringstream arg1(argv[1]);
   std::istringstream arg2(argv[2]);


   vector<vector<int>> assigned_piles;
   vector<int> assigned_remaining;

   assigned_piles = init_distribution();
   auto start_pos = init_pos(assigned_piles);

   assigned_remaining = init_remaining(assigned_piles);

   if (sums[n_cubes-1]%n_piles!=0){
       cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
       return 1;
   }

   vector<int> pile(n_cubes, false);
   vector<int> disallowed(n_cubes, false);
   vector<short> history;
   vector<thread> pile_threads;

   for(int i=0;i<n_piles;i++){
       queues.emplace_back();
   }

   auto start = chrono::high_resolution_clock::now();

   pile_threads.emplace_back(start_source, assigned_remaining[0],  assigned_piles[0]);

   for(int i=0; i<=n_piles-2;i++){
       for(int n=0; n<5; n++){
           pile_threads.emplace_back(start_thread,assigned_remaining[i+1], i, i+1, assigned_piles[i+1], start_pos);
       }
   }

//    auto m = thread(monitor);

   for(auto & pile_thread : pile_threads){
       pile_thread.join();
   }
//    m.join();

   auto stop = chrono::high_resolution_clock::now();
   auto duration = duration_cast<chrono::milliseconds>(stop-start);

   cout<<endl<<"Completed in "<<duration.count()<<" milliseconds"<<endl;

   vector<int> piles;

   if (queues[n_piles-1].try_dequeue(piles)){
       print_piles(piles);
   }else{
       cout<<"No piles found"<<endl;
   }*/
