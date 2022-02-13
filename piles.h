//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include "defs.h"
#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;

int sum_pile(vector<bool> pile);
void print(vector<vector<int>> piles);
void print_bool(bitset<n_cubes*n_piles> piles);
bool place(bitset<n_cubes*n_piles> &piles, vector<int> &remaining, int pos);

vector<vector<bool>> init_distribution();
int init_pos(vector<vector<bool>> piles);
vector<int> init_remaining(vector<vector<bool>> piles);
#endif //PILES_PILES_H
