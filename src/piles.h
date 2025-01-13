//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "defs.h"

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;

void print_pile(vector<int> pile);
void print_piles(vector<int> piles, int n_piles);

int next_allowed(int pos, int &remaining, vector<int> &disallowed);
vector<vector<int>> make_pile(int target, int remaining, int pos,
                            vector<int> &pile, vector<int> &disallowed);
int calc_remaining(vector<int> disallowed, int n_cubes);

int sum_pile(vector<int> pile);

void start_source(int target, vector<int>assigned_pile);
void start_thread(int target, int source_queue, int dest_queue, vector<int>assigned_pile, int start_pos);

void monitor();
bool is_done();

vector<vector<int>> init_distribution(int n_piles, int n_cubes);
vector<int> init_remaining(vector<vector<int>> piles, int n_piles);
int init_pos(vector<vector<int>> piles);

//vector<int> make_disallowed(vector<short> &history);

#endif //PILES_PILES_H
