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
extern vector<BlockingConcurrentQueue<vector<vector<bool>>>> queues;

#endif //PILES_DEFS_H
