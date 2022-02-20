//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "blockingconcurrentqueue.h"
#include <unordered_map>

#ifndef PILES_DEFS_H
#define PILES_DEFS_H
using namespace moodycamel;
using namespace std;

extern const int sums[];
extern const int cubes[];
extern vector<BlockingConcurrentQueue<vector<array<int, 60>>>> queues;
extern vector<array<int, 60>> assigned_piles;
extern vector<int> assigned_remaining;

extern bool stop;
extern vector<bool> possible_cubes;
extern const int max_lookup_bits;

#endif //PILES_DEFS_H
