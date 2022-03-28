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

void print_pile(vector<int_fast8_t> pile);
void print_piles(vector<int_fast8_t> piles, string filename);

int next_allowed(int pos, int &remaining, vector<int_fast8_t> &disallowed);
void make_pile(int target, int remaining, int pos, int pile_num, vector<int_fast8_t> &pile, BlockingConcurrentQueue<vector<int_fast8_t>> &dest_queue);

int calc_remaining(vector<int_fast8_t> piles, int pile);

int sum_pile(vector<int_fast8_t> pile, int pile_num);
vector<vector<dynamic_bitset<>>> read_filters(string filename, int n_cubes);

void start_source(int target, vector<int_fast8_t>assigned_pile);
void start_thread(int target, int source_queue, int dest_queue, vector<int_fast8_t>assigned_pile, int start_pos);

void monitor();
bool is_done();
void write_pile(int pile_num, int source_queue_idx, std::string filename, bool text);
void read_pile(std::string filename, int pile_num, int dest_queue_idx);

void solve(int source_queue_idx, int dest_queue_idx, int pile_num);

vector<int_fast8_t> init_distribution();
vector<int_fast8_t> init_remaining(vector<vector<int_fast8_t>> piles);
int init_pos(vector<vector<int>> piles);

void success_bloom(int pos, int pile_num, vector<int_fast8_t> &pile, BlockingConcurrentQueue<vector<int_fast8_t>> &dest_queue);

//vector<int> make_disallowed(vector<short> &history);

#endif //PILES_PILES_H
