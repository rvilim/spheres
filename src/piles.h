//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include <array>
#include <map>
#include "bitfiltertree.h"

#ifndef PILES_PILES_H
#define PILES_PILES_H

using namespace std;

class PileSolver {
private:
    const size_t memoization_limit;
    std::map<int, std::vector<__int128>> precalculated_sums;
    const std::array<int, 100> sums;
    const std::array<int, 100> cubes;
    std::unique_ptr<BitFilterTree> filter_tree;
    const bool enable_memoize;
    const bool enable_diophantine;
    const size_t n_cubes;
    const size_t n_piles;

public:
    PileSolver(int num_piles = 3, 
               int num_cubes = 23, 
               bool do_memoize = true,
               bool do_diophantine = true,
               const std::string& tree_path = "tree.bin", 
               int memoization_limit = 27);
    static constexpr std::array<int, 100> make_sums();
    static constexpr std::array<int, 100> make_cubes();
    void initialize_memoization();
    
    // Core pile manipulation functions
    vector<vector<int>> make_pile(int target, int remaining, int pos,
                                vector<int>& pile, __int128 disallowed);
    int calc_remaining(__int128 disallowed);
    int sum_pile(vector<int> pile);

    // Initialization functions
    vector<vector<int>> init_distribution();
    vector<int> init_remaining(vector<vector<int>> piles);
    int init_pos(vector<vector<int>> piles);

    void build_diophantine_tree(const string& csv_path = "diophantine_small.txt", const string& tree_path = "tree.bin", int max_depth = 40, int min_patterns_leaf=1);
    bool classify_pattern(const vector<int>& pile) const;

private:
    vector<__int128> find_valid_patterns(int target, __int128 disallowed);
};

#endif //PILES_PILES_H
