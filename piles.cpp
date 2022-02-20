#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include "defs.h"
#include <cstdlib>
#include <unordered_map>
#include <bitset>

using namespace std;
using namespace moodycamel;

const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

bool stop=false;
int max_lookup = 0;


int update_remaining(int pos, int next_pos, int remaining){
    for(int i=pos;i>next_pos;i--){
        remaining-=cubes[pos];
    }

    if (remaining<0){
        return 0;
    }

    return remaining;
}

vector<bool> possible_cubes;
/*
void cube_possible(){
    possible_cubes = vector<bool>(sums[max_lookup_bits-1], false );


    for(int num=1;num < 2<<(max_lookup_bits-1) ;num++){

        int sum=0;
        for(int pos=0;pos<max_lookup_bits;pos++){
            if ((num >> pos) & 1){
                sum+=cubes[pos];
            }
        }
        possible_cubes[sum]=true;
    }
    cout<<possible_cubes.size()<<endl;
    max_lookup=possible_cubes.size()-1;
//    for(int i=0;i<100;i++){
//        cout<<i<<" "<<possible_cubes[i]<<endl;
//    }
}
*/
int sum_pile(array<int, 60> pile){
    int s=0;

    auto n_cubes=pile.size();
    for(int pos=0; pos<n_cubes; pos++){
        s+=pile[pos]*cubes[pos];
    }

    return s;
}

void print_pile(array<int, 60> pile){
    for(auto p : pile){
        cout<<p;
    }
    cout<<" -> "<<sum_pile(pile);
    cout<<endl;
}

void print_piles(vector<array<int, 60>> piles) {
    for(auto p : piles) {
        print_pile(p);
    }


    for(int pos=0; pos<piles[0].size(); pos++){
        int s=0;
        for(auto & pile : piles){
            s +=pile[pos];
        }
        if (s==1) {
            cout<<" ";
        }else{
            cout<<"\033[1;31mx\033[0m";
        }
    }
    cout<<endl;
}


int get_next_pos(int target){
    return floor(cbrt(target))-1;
}

void make_pile(int target, int remaining, int pos,
               array<int, 60> &pile, array<int, 60> &disallowed, vector<array<int, 60>> &history,
               int queue_index,
               int n_piles, int n_cubes){

    auto cube=cubes[pos];

    if ((not disallowed[pos]) && cube==target) { // Success! we have a pile
        pile[pos] = true;

        if (queue_index==0){
            cout<<"----"<<endl;
            print_pile(pile);
        }
        vector<array<int, 60>> result(history);
        result.emplace_back(pile);
        queues[queue_index].enqueue(result);

        if(queue_index==n_piles-1){
            stop=true;
        }

        pile[pos] = false;
        return;
    }

    if (is_done()) return;

    if (pos==0){ // Can't find a pile
        return;
    }else{ // Hope is still alive
        if ((target > cube) && (!disallowed[pos])){
            pile[pos] = true;

            int next_pos = get_next_pos(target-cube);
            if (next_pos==pos) next_pos=pos-1;
//            cout<<target<<" "<<target-cube<<" "<<pos<<" "<<next_pos<<endl;
            next_pos=pos-1;
            if (next_pos<pos){
                make_pile(target - cube, remaining - cube, next_pos, pile, disallowed, history, queue_index, n_piles, n_cubes);
            }
//            make_pile(target - cube, remaining - cube, pos-1, pile, disallowed, history, queue_index, n_piles, n_cubes);

            pile[pos] = false;
        }
        make_pile(target, remaining, pos-1, pile, disallowed, history, queue_index, n_piles, n_cubes);
    }
}


array<int, 60> make_disallowed(vector<array<int, 60>> &history){
    array<int, 60> disallowed=history[0];

    for (int i=1; i<history.size(); i++){
        for(int j=0; j<disallowed.size(); j++){
            disallowed[j] = disallowed[j] | history[i][j];
        }
    }

    return disallowed;
}


vector<array<int, 60>> init_distribution(int n_cubes, int n_piles){
    int target = sums[n_cubes-1]/n_piles;

    vector<array<int, 60>> piles;
    
    for (int i=0; i<n_piles; i++){
        array<int,60> p;
        p.fill(false);
        piles.push_back(p);
    }

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

vector<int> init_remaining(int n_cubes, int n_piles, vector<array<int, 60>> piles){

    vector<int> remaining={};

    for(auto & pile : piles){
        remaining.emplace_back(sums[n_cubes - 1] / n_piles - sum_pile(pile));
    }

    return remaining;
}

vector<int> init_starts(vector<array<int, 60>> assigned_disallowed, int n_cubes){
    vector<int> assigned_start(assigned_disallowed.size(),0);

    for(int pile_num=0;pile_num<assigned_disallowed.size(); pile_num++){
        for(int i=n_cubes-1;i>=0;i--){
            if (!assigned_disallowed[pile_num][i]){
                assigned_start[pile_num]=i;
                break;
            }
        }
    }
    return assigned_start;
}

int init_pos(vector<array<int, 60>> piles){
    int pos=0;
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

void start_source(int target, int n_piles, int n_cubes, array<int, 60> assigned_pile){
    auto remaining = sums[n_cubes-1];

    array<int, 60> disallowed;
    disallowed.fill(false);

    vector<array<int, 60>> history;
    make_pile(target, remaining,n_cubes-1, assigned_pile, disallowed, history, 0, n_piles, n_cubes);

}

void start_thread(int target, int n_piles, int n_cubes, int source_queue, int dest_queue, array<int, 60>assigned_pile){
    auto start_pos = init_pos(assigned_piles);
    while(true){
        if (is_done()) return;

        vector<array<int, 60>> history;
        auto ret = queues[source_queue].wait_dequeue_timed(history, std::chrono::milliseconds(100));

        if ((!ret) && (is_done())) return;

        if (ret){
            auto disallowed = make_disallowed(history);
            make_pile(target, target,start_pos, assigned_pile, disallowed, history, dest_queue, n_piles, n_cubes);
        }

    }
}

bool is_done(){
    return stop;
}

void monitor(){
    while (true){
        system("clear");
        for(int i=0; i<queues.size();i++){
            std::cout<<i<<" "<<queues[i].size_approx()<<endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 1 second
        if (is_done()){
            return;
        }
    }

}