//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "blockingconcurrentqueue.h"
#include <set>
#include <boost/dynamic_bitset.hpp>

#ifndef PILES_DEFS_H
#define PILES_DEFS_H
using namespace moodycamel;
using namespace std;
using namespace boost;

extern const int sums[];
extern const int cubes[];
extern int n_piles;
extern int n_cubes;

extern vector<BlockingConcurrentQueue<vector<int_fast8_t>>> queues;
extern atomic<bool> stop;

extern std::set<boost::dynamic_bitset<>> masks;
extern vector<vector<dynamic_bitset<>>> filters;
#endif //PILES_DEFS_H
