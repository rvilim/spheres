#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <string>

using namespace std;
BlockingConcurrentQueue<vector<int>> source_queue, dest_queue;

int n_cubes=77;
int n_piles=13;
int pile_num;
int work_threads=8;

string out_file;
string in_file;

int read_cmd(int argc, char*argv[]);

int read_cmd(int argc, char*argv[]){
    std::istringstream arg1(argv[1]); // n_piles
    std::istringstream arg2(argv[2]); // n_cubes
    std::istringstream arg3(argv[3]); // piles_num

    out_file = argv[4];

    if (!(arg1 >> n_piles)) {
        std::cerr << "Invalid number: " << argv[1] << '\n';
    } else if (!arg1.eof()) {
        std::cerr << "Trailing characters after number: " << argv[1] << '\n';
    }

    if (!(arg2 >> n_cubes)) {
        std::cerr << "Invalid number: " << argv[2] << '\n';
    } else if (!arg2.eof()) {
        std::cerr << "Trailing characters after number: " << argv[2] << '\n';
    }

    if (!(arg3 >> pile_num)) {
        std::cerr << "Invalid number: " << argv[3] << '\n';
    } else if (!arg3.eof()) {
        std::cerr << "Trailing characters after number: " << argv[3] << '\n';
    }

    if (argc==6) {
        in_file = argv[5];
    }
}
int main(int argc,char *argv[]) {
    read_cmd(argc, argv);

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

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
}
