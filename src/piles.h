//
// Created by Ryan Vilim on 2/12/22.
//
#include <vector>
#include <array>
#include <map>
#include <nanobind/ndarray.h>
#include "bitfiltertree.h"

#ifndef PILES_PILES_H
#define PILES_PILES_H

namespace nb = nanobind;  // Add this line to use nb:: shorthand

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
    std::vector<__uint128_t> preassigned_piles;
    std::vector<int> preassigned_remaining;
    std::vector<__uint128_t> preassigned_disallowed;

public:
    PileSolver(size_t num_piles = 3, 
               size_t num_cubes = 23, 
               bool do_memoize = true,
               bool do_diophantine = true,
               const std::string& tree_path = "tree.bin", 
               size_t memoization_limit = 27,
               const std::string& memoization_path = "memo.bin");
    static constexpr std::array<int, 100> make_sums();
    static constexpr std::array<int, 100> make_cubes();
    void initialize_memoization();
    
    // Core pile manipulation functions
    vector<__uint128_t> make_pile(int target, int remaining, int pos,
                                __uint128_t pile, __int128 disallowed);
    int calc_remaining(__int128 disallowed);
    int sum_pile(__uint128_t pile);

    // Initialization functions
    vector<int> init_distribution();
    vector<int> init_remaining(vector<__uint128_t> piles);
    int init_pos(vector<__uint128_t> piles);

    void build_diophantine_tree(const string& csv_path = "diophantine_small.txt", const string& tree_path = "tree.bin", int max_depth = 40, int min_patterns_leaf=1);
    bool classify_pattern(__uint128_t pile) const;
    vector<vector<int>> solve_from_assignment(const nb::ndarray<int> assignments,
                                     int target_pile_num,
                                     size_t num_threads = 1);
private:
    vector<__int128> find_valid_patterns(int target, __int128 disallowed);
    bool load_memoization(const std::string& path);
    void save_memoization(const std::string& path);
};

#endif //PILES_PILES_H
