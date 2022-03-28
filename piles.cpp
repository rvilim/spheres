#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include "defs.h"
#include <fstream>
#include <cassert>
#include <cinttypes>
#include <sstream>
#include "BloomFilter.h"
#include <boost/dynamic_bitset.hpp>

using namespace boost;
using namespace std;
using namespace moodycamel;

const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

atomic<bool> stop=false;
unsigned int n_solutions=0;
unsigned int read_progress=0;
unsigned int n_records=0;
mutex mask_lock;

std::set<boost::dynamic_bitset<>> masks;
BloomFilter masks_bloom(3310561880, 10);

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}

vector<vector<dynamic_bitset<>>> read_filters(string filename, int n_cubes){
    vector<vector<dynamic_bitset<>>> filters(100);
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        dynamic_bitset<> lhs(n_cubes);
        int8_t rhs;

        {
            auto pair = split(string(line), '|');
            auto lhs_s = split(pair[0], ',');

            rhs = stoi(pair[1]);

            if (rhs>n_cubes) continue;

            for(auto & i : lhs_s){
                if (stoi(i)>n_cubes) continue;
                lhs[stoi(i)-1]=1;
            }
        }
        filters[rhs].push_back(lhs);
    }
    infile.close();

    return filters;
}


int calc_remaining(vector<int_fast8_t> piles, int pile){
    int remaining=sums[n_cubes-1];

    for (int pos = 0; pos < piles.size(); pos++)
        if (piles[pos]!=0) {
            remaining -= cubes[pos];
        };
    return remaining;
}

void print_piles(vector<int_fast8_t> disallowed, string filename) {
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

    for (int pile=1; pile<n_piles;pile++){
        if (cubes[n_cubes-pile]+cubes[n_cubes-pile-1]>target){

            piles[n_cubes-pile]=pile;

        }
    }

    return piles;
}

bool filter_bloom(vector<int_fast8_t> &pile){
    vector<uint_fast8_t> mask(n_cubes, 0);

    for(int i=0;i<pile.size();i++){
        if (pile[i]!=0){
            mask[i]=1;
        }
    }

    auto m = mask.data();

    // Here we add the overall mask to a boom filter (e.g. pile independent).
    // If we've already seen this mask there is no point in continuing on

    {
        mask_lock.lock();
        if (!masks_bloom.possiblyContains(m, mask.size())) {
            masks_bloom.add(m, mask.size());
            mask_lock.unlock();
            return false;

        }else{
            mask_lock.unlock();
            return true;
        }
    }
}
void success(int pos, int pile_num, vector<int_fast8_t> &pile, BlockingConcurrentQueue<vector<int_fast8_t>> &dest_queue){
    if ((!filter_bloom(pile))){
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

    auto init = init_distribution();
    auto target = sums[n_cubes-1]/n_piles - sum_pile(init, pile_num);

    while(true){
        auto ret = queues[source_queue_idx].wait_dequeue_timed(pile, std::chrono::milliseconds(100));
        if (is_done()) return;

        if (!ret) continue;

        auto remaining = calc_remaining(pile,pile_num);
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
        cout<<masks.size()<<endl;
        if (is_done()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

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
            auto ret = queues[source_queue_idx].wait_dequeue_timed(pile, std::chrono::milliseconds(10));

            if (!ret) {
                // If all our queues are empty set stop=True
                stop = true;

                for(auto & queue : queues){
                    stop = stop && (queue.size_approx()==0);
                }
            }

            if (is_done()) {
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