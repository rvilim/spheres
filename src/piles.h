//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include <array>

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;

// Global constants
extern const std::array<int, 100> sums;
extern const std::array<int, 100> cubes;

// Initialize memoization tables
void initialize_memoization();
void init_module();

// Core pile manipulation functions
vector<vector<int>> make_pile(int target, int remaining, int pos,
                            vector<int>& pile, __int128 disallowed);
int calc_remaining(__int128 disallowed, int n_cubes);
int sum_pile(vector<int> pile);

// Initialization functions
vector<vector<int>> init_distribution(int n_piles, int n_cubes);
vector<int> init_remaining(vector<vector<int>> piles, int n_piles);
int init_pos(vector<vector<int>> piles);

#endif //PILES_PILES_H
