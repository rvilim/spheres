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
void print_piles(vector<int> piles);

int next_allowed(int pos, int &remaining, vector<int> &disallowed);
void make_pile(int target, int remaining, int pos, int pile_num, vector<int> &pile);

int calc_remaining(vector<int> piles, int pile);

int sum_pile(vector<int> pile, int pile_num);

void start_source(int target, vector<int>assigned_pile);
void start_thread(int target, int source_queue, int dest_queue, vector<int>assigned_pile, int start_pos);

void monitor();
bool is_done();
void write(std::string filename, int pile_num);
void read(std::string filename, int pile_num);

void solve(int pile_num);

vector<int> init_distribution();
vector<int> init_remaining(vector<vector<int>> piles);
int init_pos(vector<vector<int>> piles);

//vector<int> make_disallowed(vector<short> &history);

#endif //PILES_PILES_H
