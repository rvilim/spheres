#include <iostream>
#include <utility>
#include <vector>
#include <stack>
#include <thread>
#include <chrono>
#include "piles.h"
#include "defs.h"
#include <cstdlib>

using namespace std;
using namespace moodycamel;

void Pile::success(int pos, vector<int> &pile, vector<int> &disallowed){

    pile[pos] = true;

    vector<int> result(disallowed);

    for(int i=0;i<pile.size(); i++){
        if (pile[i]!=0){
            result[i]=pile_number+1;
        }
    }

    while (dest_queue->size_approx()>10000000){ // Don't let too many stack up on the queue, or we will OOM
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

//    if (pile_number>=4 || !diophantine_filter->find(pile)) {
    dest_queue->enqueue(result);
    queue_stats->n_queued+=1;
//    }

    if(pile_number==n_piles-1){
        *stop= true;
    }
    pile[pos] = false;
}

void Pile::make_pile(int target, int remaining, int pos,
               vector<int> &pile, vector<int> &disallowed){

    if (disallowed[pos]) { // If the one we are on is disallowed, just skip it.
        if (pos==0) return;
        make_pile(target, remaining, pos - 1, pile, disallowed);
        return;
    }

    if (target>remaining) {
        return;
    }

    if (cubes[pos]==target) { // Success! we have a pile
        success( pos,  pile, disallowed);
        return;
    }

    if (pos==0) return;


    // Call the function again with the bit in question both set and unset
    if (target-cubes[pos]>0) {
        pile[pos] = true;
        make_pile(target-cubes[pos], remaining-cubes[pos], pos-1,  pile, disallowed);
        pile[pos] = false;
    }
    make_pile(target, remaining-cubes[pos], pos-1, pile, disallowed);

}

vector<vector<int>> init_distribution(int n_cubes, int n_piles){
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

vector<int> init_remaining(vector<vector<int>> piles, int n_cubes, int n_piles){

    vector<int> remaining={};

    for(auto & pile : piles){
        remaining.emplace_back(sums[n_cubes - 1] / n_piles - sum_pile(pile, n_cubes));
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

int calc_remaining(vector<int> disallowed, int n_cubes){
    int remaining=sums[n_cubes-1];

    for (int pos = 0; pos < disallowed.size(); pos++)
        if (disallowed[pos]) {
            remaining -= cubes[pos];
        };
    return remaining;
}


void start_source(Pile *pile, int target, int n_cubes, vector<int> assigned_pile){
    vector<int> disallowed(n_cubes, false);

//    pile->make_pile_iter(target, sums[n_cubes-1], n_cubes-2, assigned_pile, disallowed);
    pile->make_pile(target, sums[n_cubes-1], n_cubes-2, assigned_pile, disallowed);

}

void start_thread(Pile *pile, int target, int n_cubes, vector<int>assigned_pile, int start_pos){
    const size_t N_DEQUEUE=5;
    vector<int> disallowed[N_DEQUEUE];

    while(true){
        if (pile->is_done()) return;
        auto n = pile->source_queue->try_dequeue_bulk(disallowed, N_DEQUEUE);

        if (n==0){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }else{
            (*pile->queue_stats).n_deqeueued+=n;
            for (auto i = n; i != 0; --i) {
                auto remaining = calc_remaining(disallowed[i-1], n_cubes);
                pile->make_pile(target, remaining, start_pos, assigned_pile, disallowed[i-1]);
            }
        }
    }
}

bool Pile::is_done(){
    return *stop;

}

void monitor(vector<BlockingConcurrentQueue<vector<int>>> *queues, atomic_bool *global_stop, vector<QueueStats> *queue_stats){
    while (true){
        system("clear");
        for(int i=0; i<queues->size();i++){
            std::cout<<i<<" "<<(*queues)[i].size_approx()<<" n_dequeued: "<< (*queue_stats)[i].n_deqeueued<<" n_enqueued: "<< (*queue_stats)[i].n_queued<<endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (*global_stop){
            return;
        }
    }
}

int sum_pile(vector<int> pile, int n_cubes){
    int s=0;

    for(int pos=0; pos<n_cubes; pos++){
        s+=pile[pos]*cubes[pos];
    }

    return s;
}

void print_piles(vector<int> piles, int n_piles) {

    for(int pile=1; pile<=n_piles;pile++){
        int sum=0;
        for(int i=0; i<piles.size(); i++){
            if (pile==piles[i]){
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
