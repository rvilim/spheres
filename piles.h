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

void make_pile(int target, int pos, vector<int> &interleaved, vector<vector<int>> &history, int queue_index);
int sum_pile(vector<int> pile);

void start_source(int target, vector<int>assigned_pile);
void start_thread(int target, int source_queue, int dest_queue, vector<int>assigned_pile);

void monitor();
bool is_done();

vector<vector<int>> init_distribution();
vector<int> init_remaining(vector<vector<int>> piles);
int init_pos(vector<vector<int>> piles);

vector<int> make_disallowed(vector<vector<int>> &history);

#endif //PILES_PILES_H
