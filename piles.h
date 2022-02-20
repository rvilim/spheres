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

void print_pile(vector<int> pile);
void print_piles(vector<vector<int>> piles);

void make_pile(int target, int pos,
               vector<int> &pile, vector<int> &disallowed, vector<vector<int>> &history,
               int queue_index, int n_piles, int n_cubes);
int sum_pile(vector<int> pile);

void start_source(int target, int n_piles, int n_cubes, vector<int>assigned_pile);
void start_thread(int target, int n_piles, int n_cubes, int source_queue, int dest_queue, vector<int>assigned_pile);

void monitor();
bool is_done();

vector<vector<int>> init_distribution(int n_cubes, int n_piles);
vector<int> init_remaining(int n_cubes, int n_piles, vector<vector<int>> piles);
int init_pos(vector<vector<int>> piles);

vector<int> make_disallowed(vector<vector<int>> &history);

#endif //PILES_PILES_H
