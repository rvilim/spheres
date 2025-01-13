#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include "defs.h"
#include <cstdlib>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// auto n_cubes=23;
// auto n_piles=3;
using namespace std;

// const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
// const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

constexpr std::array<int, 100> make_sums() {
    std::array<int, 100> arr = {};
    for (int n = 1; n <= 100; n++) {
        arr[n-1] = ((n * (n + 1)) / 2) * ((n * (n + 1)) / 2);
    }
    return arr;
}

constexpr std::array<int, 100> make_cubes() {
    std::array<int, 100> arr = {};
    for (int i = 1; i <= 100; i++) {
        arr[i-1] = i * i * i;
    }
    return arr;
}

constexpr auto sums = make_sums();
constexpr auto cubes = make_cubes();

int sum_pile(vector<int> pile){
    int s=0;

    for(int pos=0; pos<pile.size(); pos++){
        s+=pile[pos]*cubes[pos];
    }

    return s;
}

void print_piles(vector<int> disallowed, int n_piles) {

    for(int pile=1; pile<=n_piles;pile++){
        int sum=0;
        for(int i=0; i<disallowed.size(); i++){
            if (pile==disallowed[i]){
                cout<<"1";
                sum+=cubes[i];
            } else{
                cout<<"0";
            }
        }
        cout<<" "<<sum<<endl;
    }
    cout<<endl;
}



void success(int pos, vector<int> &pile, vector<int> &disallowed){
    pile[pos] = true;

    vector<int> result(disallowed);

    // for(int i=0;i<pile.size(); i++){
    //     if (pile[i]!=0){
    //         result[i]=queue_index+1;
    //     }
    // }
    for(int i=0; i<pile.size(); i++) {
        cout << pile[i];
    }
    cout << endl;
//    if (queues[queue_index].size_approx()>1000000){
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    }
    // queues[queue_index].enqueue(result);

    // if(queue_index==n_piles-1){
    //     stop=true;
    // }
    pile[pos] = false;
}

// queue index 3
// Target: 137674
// Remaining: 2044153
// Pile: 00000000000000000000000000000000000000000000000000000001000
// Disallowed: 12223113031012033230002000000000000000000000021300000000321


void make_pile_old(int target, int remaining, int pos,
               vector<int> &pile, vector<int> &disallowed){

//    cout<<target<<endl;
    if (disallowed[pos]) { // If the one we are on is disallowed, just skip it.
        if (pos==0) return;
        make_pile_old(target, remaining, pos - 1, pile, disallowed);
        return;
    }

    if (target>remaining) {
        return;
    }

    if (cubes[pos]==target) { // Success! we have a pile
        success( pos, pile, disallowed);
        return;
    }

    if (pos==0) return;
//    auto next_pos = next_allowed(pos, remaining, disallowed);
//    if (next_poes==-1) return

    // Call the function again with the bit in question both set and unset
    if (target-cubes[pos]>0) {
        pile[pos] = true;
        make_pile_old(target-cubes[pos], remaining-cubes[pos], pos-1,  pile, disallowed);
        pile[pos] = false;
    }
    make_pile_old(target, remaining-cubes[pos], pos-1, pile, disallowed);

}

vector<vector<int>> make_pile(int target, int remaining, int pos,
                            vector<int> &pile, vector<int> &disallowed) {
    vector<vector<int>> solutions;
    
    if (disallowed[pos]) {
        if (pos == 0) return solutions;
        return make_pile(target, remaining, pos - 1, pile, disallowed);
    }

    if (target > remaining) {
        return solutions;
    }

    if (cubes[pos] == target) {
        pile[pos] = true;
        solutions.push_back(pile);
        pile[pos] = false;
        return solutions;
    }

    if (pos == 0) return solutions;

    
    // Try setting the current position
    if (target - cubes[pos] > 0) {
        pile[pos] = true;
        auto sub_solutions = make_pile(target - cubes[pos], remaining - cubes[pos], pos - 1, pile, disallowed);
        solutions.insert(solutions.end(), sub_solutions.begin(), sub_solutions.end());
        pile[pos] = false;
    }

    // Try without setting the current position
    auto more_solutions = make_pile(target, remaining - cubes[pos], pos - 1, pile, disallowed);
    solutions.insert(solutions.end(), more_solutions.begin(), more_solutions.end());

    return solutions;
}

int next_allowed(int pos, int &remaining, vector<int> &disallowed){
    for(int i=pos-1;i>=0;i--){
        remaining-=cubes[i+1];
        if (!disallowed[i]) return i;
    }
    return -1;
}

vector<vector<int>> init_distribution(int n_piles, int n_cubes){
    int target = sums[n_cubes-1]/n_piles;

    vector<vector<int>> piles(n_piles, vector<int>(n_cubes, false));
    piles[0][n_cubes-1]=true;

    for (int pile_num=1; pile_num<n_piles; pile_num++){
        // The condition here is that we can place things without loss of generality
        // as long as two adjacent cubes add up to greater than the target. As soon
        // as that's not true the smaller could either go in a new pile, or double up
        // with an already placed pile, and we have to leave it to the main solver to
        // figure tha one out.
        if (cubes[n_cubes-pile_num]+cubes[n_cubes-pile_num-1]>target){
            piles[pile_num][n_cubes-pile_num-1]=true;
        }
    }

    return piles;
}

vector<int> init_remaining(vector<vector<int>> piles, int n_piles){

    vector<int> remaining={};
    auto n_cubes = piles[0].size();
    for(auto & pile : piles){
        remaining.emplace_back(sums[n_cubes - 1] / n_piles - sum_pile(pile));
    }

    return remaining;
}

int init_pos(vector<vector<int>> piles){
    for (int i=piles.size()-1; i>=0;i--){
        auto pile = piles[i];

        for(int j=0; j<pile.size(); j++){
            if (pile[j]){
                return j-1;
            }
        }
    }
    cout<<"yikes init_pos"<<endl;
    return -1;
}

// void start_source(int target, vector<int> assigned_pile){
//     vector<int> disallowed(n_cubes, false);

//     make_pile(target, sums[n_cubes-1], n_cubes-2, assigned_pile, disallowed,  0);

// }

int calc_remaining(vector<int> disallowed, int n_cubes){
    int remaining=sums[n_cubes-1];

    for (int pos = 0; pos < disallowed.size(); pos++)
        if (disallowed[pos]) {
            remaining -= cubes[pos];
        };
    return remaining;
}


int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(piles, m) {
    m.doc() = R"pbdoc(
        Pybind11 piles
        -----------------------

        .. currentmodule:: piles

        .. autosummary::
           :toctree: _generate

           init_pos
           init_distribution
           make_pile
    )pbdoc";

    m.def("init_pos", &init_pos, R"pbdoc(init pos)pbdoc");
    m.def("init_distribution", &init_distribution, R"pbdoc(init distribution)pbdoc");
    m.def("init_remaining", &init_remaining, R"pbdoc(
        Initialize the remaining values for each pile
    )pbdoc");
    m.def("calc_remaining", &calc_remaining, R"pbdoc(
        Calculate the remaining sum after removing disallowed cubes
    )pbdoc");
    m.def("make_pile", &make_pile, R"pbdoc(
    make that pile
    )pbdoc");
    m.def("make_pile_old", &make_pile_old, R"pbdoc(
    make that pile
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}