//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "defs.h"
#include "ThreadSafeQueue.h"

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;

void print_pile(vector<bool> pile);
void make_pile(int target, int remaining, int pos, vector<bool> &pile, vector<bool> &disallowed, ThreadsafeQueue<vector<bool>> &accumulator, int n_piles, int n_cubes);
int sum_pile(vector<bool> pile);

#endif //PILES_PILES_H
