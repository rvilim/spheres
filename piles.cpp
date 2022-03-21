#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include "defs.h"
#include <fstream>
#include <assert.h>
#include <cinttypes>
#include <sstream>

using namespace std;
using namespace moodycamel;

const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

bool stop=false;
unsigned int n_solutions=0;
unsigned int read_progress=0;
unsigned int n_records=0;

std::mutex g_mask_mutex;
std::set<boost::dynamic_bitset<>> masks;

int calc_remaining(vector<int_fast8_t> piles, int pile){
    int remaining=sums[n_cubes-1];

    for (int pos = 0; pos < piles.size(); pos++)
        if (piles[pos]==pile) {
            remaining -= cubes[pos];
        };
    return remaining;
}

void print_piles(vector<int_fast8_t> disallowed, string filename="") {
    ostringstream s;

    for(int pile=1; pile<=n_piles;pile++){
        int sum=0;
        for(int i=0; i<disallowed.size(); i++){
            if (pile==disallowed[i]){
                s<<"1";
                sum+=cubes[i];
            } else{
                s<<"0";
            }
        }
        s<<" "<<sum<<endl;
    }
    s<<endl;

    if(filename!=""){
        std::ofstream out(filename);
        out << s.str();
        out.close();
    }else{
        cout<<s.str();
    }
}


vector<int_fast8_t> init_distribution(){
    int target = sums[n_cubes-1]/n_piles;

    vector<int_fast8_t> piles(n_cubes, 0);
    piles[n_cubes-1]=1;

    for (int pile=1; pile<n_piles;pile++){
        if (cubes[n_cubes-pile]+cubes[n_cubes-pile-1]>target){

            piles[n_cubes-pile]=pile;

        }
    }

    return piles;
}


void success(int pos, int pile_num, vector<int_fast8_t> &pile, BlockingConcurrentQueue<vector<int_fast8_t>> &dest_queue){
    boost::dynamic_bitset<> mask(n_cubes);

    for(int i=0;i<pile.size();i++){
        if (pile[i]!=0){
            mask[i]=true;
        }
    }

    if (masks.find(mask)==masks.end()){
        {
            std::lock_guard<std::mutex> guard(g_mask_mutex);
            masks.insert(mask);
        }

        pile[pos] = pile_num;
        dest_queue.enqueue(pile);
        pile[pos] = 0;
    }

}


void make_pile(int target, int remaining, int pos, int pile_num, vector<int_fast8_t> &pile, BlockingConcurrentQueue<vector<int_fast8_t>> &dest_queue){

    if (pile[pos]!=0) { // If the one we are on is disallowed, just skip it.
        if (pos==0) return;
        make_pile(target, remaining, pos - 1, pile_num, pile, dest_queue);
        return;
    }

    if (target>remaining) {
        return;
    }

    if (cubes[pos]==target) { // Success! we have a pile
        success( pos,  pile_num, pile, dest_queue);
        return;
    }

    if ((pos==0) || is_done()) return;

    // Call the function again with the bit in question both set and unset
    if (target-cubes[pos]>0) {
        pile[pos] = pile_num;
        make_pile(target-cubes[pos], remaining-cubes[pos], pos-1, pile_num, pile, dest_queue);
        pile[pos] = 0;
    }
    make_pile(target, remaining-cubes[pos], pos-1, pile_num, pile, dest_queue);

}


bool is_done(){
    return stop;
}

int sum_pile(vector<int_fast8_t> pile, int pile_num){
    int s=0;

    for(int pos=0; pos<n_cubes; pos++){
        if (pile[pos]==pile_num) {
            s+=cubes[pos];
        }
    }

    return s;
}

void solve(int source_queue_idx, int dest_queue_idx, int pile_num){
    vector<int_fast8_t> pile;

    while(true){
        auto ret = queues[source_queue_idx].wait_dequeue_timed(pile, std::chrono::milliseconds(1));
        if (is_done()) return;

        if (!ret) continue;

        auto remaining = calc_remaining(pile,1);
        auto target = sums[n_cubes-1]/n_piles - sum_pile(pile, pile_num);

        make_pile(target, remaining, n_cubes-1, pile_num, pile, queues[dest_queue_idx]);
    }
}

void monitor(){
    while (true){
        if (is_done()) return;
        system("clear");
        for(int i=0;i<queues.size();i++) {
            cout<<i<<" "<<queues[i].size_approx()<<endl;
        }
        if (is_done()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
//void monitor(){
//    while (true){
//        system("clear");
//        cout<<"source queue: "<<source_queue.size_approx()<<endl;
//        cout<<"dest queue: "<<dest_queue.size_approx()<<endl;
//        cout<<"progress: "<<read_progress-source_queue.size_approx()<<"/"<<n_records<<endl;
//        cout<<"n_solutions: "<<n_solutions<<endl;
//
//        if (is_done()){
//            return;
//        }else{
//            std::this_thread::sleep_for(std::chrono::milliseconds(500));
//        }
//    }
//}

void read_pile(std::string filename, int pile_num, int dest_queue_idx) {
    std::ifstream in(filename, std::ios::binary);

    unsigned int n_piles_file, n_cubes_file, pile_num_file, int_size_bytes;

    in.read(reinterpret_cast<char*>(&n_records), sizeof(n_records));
    in.read(reinterpret_cast<char*>(&n_piles_file), sizeof(n_piles_file));
    in.read(reinterpret_cast<char*>(&n_cubes_file), sizeof(n_cubes_file));
    in.read(reinterpret_cast<char*>(&pile_num_file), sizeof(pile_num_file));
    in.read(reinterpret_cast<char*>(&int_size_bytes), sizeof(int_size_bytes));

    cout<<n_cubes_file<<" "<<n_piles_file<<" "<<pile_num_file<<" "<<n_records<<flush<<endl;

    assert(n_piles_file==n_piles);
    assert(n_cubes_file==n_cubes);
    assert(pile_num_file==pile_num-1);

    vector<int_fast8_t> pile(n_cubes, 0);

    while (true){
        if(queues[dest_queue_idx].size_approx()<100000) {
            if (in.read(reinterpret_cast<char *>(&pile[0]), int_size_bytes * n_cubes)) {
                read_progress+=1;
                queues[dest_queue_idx].enqueue(pile);
            } else {
                return;
            }
        }else{
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void write_pile(int pile_num, int source_queue_idx, std::string filename, bool text) {
    if (!text) {
        std::ofstream file(filename, std::ios::binary);
        vector<int_fast8_t> pile;

        using T = typename std::decay<decltype(*pile.begin())>::type;
        unsigned int int_size_bytes = sizeof(T);

        file.write(reinterpret_cast<const char *>(&n_solutions), sizeof(n_solutions));
        file.write(reinterpret_cast<const char *>(&n_piles), sizeof(n_piles));
        file.write(reinterpret_cast<const char *>(&n_cubes), sizeof(n_cubes));
        file.write(reinterpret_cast<const char *>(&pile_num), sizeof(pile_num));
        file.write(reinterpret_cast<const char *>(&int_size_bytes), sizeof(int_size_bytes));

        while (true) {
            auto ret = queues[source_queue_idx].wait_dequeue_timed(pile, std::chrono::milliseconds(1000));

            if ((!ret) && (is_done())) {
                // If we are closing the file, seek to the beginning and write how many solutions we have
                file.seekp(0);
                file.write(reinterpret_cast<const char *>(&n_solutions), sizeof(n_solutions));
                return;
            }

            if (ret) {
                n_solutions += 1;
                file.write(reinterpret_cast<const char *>(&pile[0]), sizeof(pile[0]) * n_cubes);
            }
        }
    } else {

        vector<int_fast8_t> pile;

        while (true) {
            auto ret = queues[source_queue_idx].wait_dequeue_timed(pile, std::chrono::milliseconds(1000));

            if (ret ) {
                if(text){
                    print_piles(pile, filename);

                }
                stop = true;

                return;
            }

        }


    }
}
/*
int sum_pile(vector<int> pile){
    int s=0;

    for(int pos=0; pos<n_cubes; pos++){
        s+=pile[pos]*cubes[pos];
    }

    return s;
}




void success(int pos, int queue_index, vector<int> &pile, vector<int> &disallowed){
    pile[pos] = true;

    vector<int> result(disallowed);

    for(int i=0;i<pile.size(); i++){
        if (pile[i]!=0){
            result[i]=queue_index+1;
        }
    }

//    if (queues[queue_index].size_approx()>1000000){
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    }
    queues[queue_index].enqueue(result);

    if(queue_index==n_piles-1){
        stop=true;
    }
    pile[pos] = false;
}



int next_allowed(int pos, int &remaining, vector<int> &disallowed){
    for(int i=pos-1;i>=0;i--){
        remaining-=cubes[i+1];
        if (!disallowed[i]) return i;
    }
    return -1;
}

vector<int> init_remaining(vector<vector<int>> piles){

    vector<int> remaining={};

    for(auto & pile : piles){
        remaining.emplace_back(sums[n_cubes - 1] / n_piles - sum_pile(pile));
    }

    return remaining;
}

int init_pos(vector<vector<int>> piles){
    auto it = piles.rbegin();

    for (int i=piles.size()-1; i>=0;i--){
        auto pile = piles[i];

        for(int j=0; j<pile.size(); j++){
            if (pile[j]){
                return j-1;
            }
        }
    }
}

void start_source(int target, vector<int> assigned_pile){
    vector<int> disallowed(n_cubes, false);

    make_pile(target, sums[n_cubes-1], n_cubes-2, assigned_pile, disallowed,  0);

}


void start_thread(int target, int source_queue, int dest_queue, vector<int>assigned_pile, int start_pos){
    while(true){
        if (is_done()) return;

        vector<int> disallowed;
        auto ret = queues[source_queue].wait_dequeue_timed(disallowed, std::chrono::milliseconds(1000));

        if ((!ret) && (is_done())) return;

        if (ret){
            auto remaining = calc_remaining(disallowed);
            make_pile(target, remaining, start_pos, assigned_pile, disallowed,  dest_queue);
        }

    }
}

void monitor(){
    while (true){
        system("clear");
        for(int i=0; i<queues.size();i++){
            std::cout<<i<<" "<<queues[i].size_approx()<<endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (is_done()){
            return;
        }
    }

}*/