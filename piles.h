//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "defs.h"
#include "blockingconcurrentqueue.h"
#include "diophantine.h"

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;
using namespace moodycamel;

struct QueueStats {
    unsigned long n_queued;
    unsigned long n_deqeueued;
};

class Pile {
public:
    Pile(int piles, int cubes, BlockingConcurrentQueue<vector<int>> &src,
         BlockingConcurrentQueue<vector<int>> &dest,  int pile_num, atomic_bool *global_stop,
         QueueStats &queue_statistics,
         Filter &filter
         ) {
        // Hack
        if (pile_num==0){
            source_queue = nullptr;
        }else{
            source_queue = &src;
        }

        dest_queue = &dest;
        pile_number = pile_num;
        stop = global_stop;

        n_cubes = cubes;
        n_piles = piles;
        queue_stats = &queue_statistics;
        diophantine_filter = &filter;
    }

    void make_pile(int target, int remaining, int pos,
                   vector<int> &pile, vector<int> &disallowed);

    void make_pile_iter(int target, int remaining, int pos,
                   vector<int> &pile, vector<int> &disallowed);

    bool is_done();
    BlockingConcurrentQueue<vector<int>> *source_queue, *dest_queue;
    QueueStats *queue_stats;
    int n_cubes, n_piles;


private:
//    Filter *diophantine_filter = new Filter("/Users/rvilim/repos/piles/cubes_6", 90);
    int pile_number;
    atomic_bool *stop;
    Filter *diophantine_filter;
    void success(int pos, vector<int> &pile, vector<int> &disallowed);
};

void start_source(Pile *pile, int target, int n_cubes, vector<int>assigned_pile);
void start_thread(Pile *pile, int target, int n_cubes, vector<int>assigned_pile, int start_pos);

void print_piles(vector<int> piles, int n_piles);
int sum_pile(vector<int> pile, int n_cubes);
int calc_remaining(vector<int> disallowed, int n_cubes);
void monitor(vector<BlockingConcurrentQueue<vector<int>>> *queues,  atomic_bool *global_stop,  vector<QueueStats> *queue_stats);

void print_pile(vector<int> pile);

vector<vector<int>> init_distribution(int n_cubes, int n_piles);
vector<int> init_remaining(vector<vector<int>> piles, int n_cubes, int n_piles);

int init_pos(vector<vector<int>> piles);

//vector<int> make_disallowed(vector<short> &history);

#endif //PILES_PILES_H
