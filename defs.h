//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "blockingconcurrentqueue.h"

#ifndef PILES_DEFS_H
#define PILES_DEFS_H
using namespace moodycamel;
using namespace std;

extern const int sums[];
extern const int cubes[];
extern int n_piles;
extern int n_cubes;
extern vector<BlockingConcurrentQueue<vector<int>>> queues;
extern bool stop;

#endif //PILES_DEFS_H
