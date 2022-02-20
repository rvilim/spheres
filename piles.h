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

void print_pile(array<int, 60> pile);
void print_piles(vector<array<int, 60>> piles);

void make_pile(int target, int remaining, int pos,
               array<int, 60> &pile, array<int, 60> &disallowed, vector<array<int, 60> > &history,
               int queue_index, int n_piles, int n_cubes);
int sum_pile(array<int, 60> pile);

void start_source(int target, int n_piles, int n_cubes, array<int, 60>assigned_pile);
void start_thread(int target, int n_piles, int n_cubes, int source_queue, int dest_queue, array<int, 60> assigned_pile);

void monitor();
bool is_done();

vector<array<int, 60> > init_distribution(int n_cubes, int n_piles);
vector<int>  init_remaining(int n_cubes, int n_piles, vector<array<int, 60> > piles);
vector<array<int, 60>> init_disallowed(vector<array<int, 60>> piles, int n_cubes);
int init_pos(vector<array<int, 60> > piles);
vector<int> init_starts(vector<array<int, 60>> assigned_disallowed, int n_cubes);

array<int, 60>  make_disallowed(vector<array<int, 60>> &history);

#endif //PILES_PILES_H
