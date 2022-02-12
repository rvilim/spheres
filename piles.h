//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;

int sum_pile(vector<bool> pile);
void print(vector<vector<int>> piles, int n_cubes);
vector<vector<int>> place(vector<vector<int>> &piles, vector<int> &remaining, int pos, int n_cubes);

vector<vector<int>> init_distribution(int n_piles, int n_cubes);
int init_pos(vector<vector<int>> piles);
vector<int> init_remaining(vector<vector<int>> piles, int n_cubes);
#endif //PILES_PILES_H
