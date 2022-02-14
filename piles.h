//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "defs.h"
#include "blockingconcurrentqueue.h"

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;
using namespace moodycamel;

void print_pile(vector<bool> pile);
void print_piles(vector<vector<bool>> piles);

void make_pile(int target, int remaining, int pos,
               vector<bool> &pile, vector<bool> &disallowed, vector<vector<bool>> &history,
               int queue_index, int n_piles, int n_cubes);
int sum_pile(vector<bool> pile);

void start_source(int target, int n_piles, int n_cubes);
void start_thread(int target, int n_piles, int n_cubes, int source_queue, int dest_queue);
void start_final(int n_piles);

void monitor();
bool is_done();

vector<bool> make_disallowed(vector<vector<bool>> &history);

#endif //PILES_PILES_H
