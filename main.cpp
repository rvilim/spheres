#include <iostream>
#include <vector>
#include "defs.h"
#include "piles.h"
#include <chrono>
#include <thread>
#include <sstream>

using namespace std;

struct Params{
    int n_cubes;
    int n_piles;
    int n_threads;
};

const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};


Params parse_input(char *argv[]){
    int n_cubes, n_piles, n_threads;

    std::istringstream arg1(argv[1]);
    std::istringstream arg2(argv[2]);
    std::istringstream arg3(argv[3]);

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

    if (!(arg3 >> n_threads)) {
        std::cerr << "Invalid number: " << argv[2] << '\n';
    } else if (!arg2.eof()) {
        std::cerr << "Trailing characters after number: " << argv[2] << '\n';
    }


    Params p = {
            n_cubes, n_piles, n_threads
    };

    return p;
}
int main(int argc, char *argv[]) {

    auto input_params = parse_input(argv);

    int n_cubes = input_params.n_cubes;
    int n_piles = input_params.n_piles;
    int n_threads = input_params.n_threads;

    vector<vector<int>> assigned_piles;
    vector<int> assigned_remaining;

    assigned_piles = init_distribution(n_cubes, n_piles);
    auto start_pos = init_pos(assigned_piles);

    assigned_remaining = init_remaining(assigned_piles, n_cubes, n_piles);

    if (sums[n_cubes-1]%n_piles!=0){
        cout<<"The sum of the first "<<n_cubes<<" cubes is not divisible by "<<n_piles<<". Not Possible ☹️"<<endl;
        return 1;
    }

    vector<thread> pile_threads;
    vector<Pile> piles;
    atomic_bool stop = false;
    vector<BlockingConcurrentQueue<vector<int>>> queues(n_piles);

    piles.emplace_back(n_piles, n_cubes, queues[0], queues[0],  0, &stop);

    for(int i=1;i<n_piles;i++){
        piles.emplace_back(n_piles, n_cubes, queues[i-1], queues[i], i, &stop);
    }

    auto start_time = chrono::high_resolution_clock::now();

    pile_threads.emplace_back(start_source, &piles[0], assigned_remaining[0], n_cubes, assigned_piles[0]);

    for(int i=1; i<n_piles;i++){
        for(int n=0; n<n_threads; n++){
            pile_threads.emplace_back(start_thread, &piles[i], assigned_remaining[i], n_cubes, assigned_piles[i], start_pos);
        }
    }

    auto m = thread(monitor, &queues, &stop);

    for(auto & pile_thread : pile_threads){
        pile_thread.join();
    }
    m.join();

    auto stop_time = chrono::high_resolution_clock::now();
    auto duration = duration_cast<chrono::milliseconds>(stop_time-start_time);

    cout<<endl<<"Completed in "<<duration.count()<<" milliseconds"<<endl;

    vector<int> p;

    if (piles[n_piles-1].dest_queue->try_dequeue(p)){
        print_piles(p, n_piles);
    }else{
        cout<<"No piles found"<<endl;
    }
    return 0;
}
