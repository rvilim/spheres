//
// Created by Ryan Vilim on 2/12/22.
//
#ifndef PILES_H
#define PILES_H

#include <vector>
#include <array>
#include <string>
#include <map>
#include <condition_variable>
#include <unordered_set>
#include <future>
#include <ranges>

#include "bitfiltertree.h"
// Wrap nanobind-specific includes
#ifdef NB_MODULE
#include <nanobind/ndarray.h>
namespace nb = nanobind;
#endif

using namespace std;

class PileSolver {
private:
    struct PileSetup {
        __uint128_t target_pile;
        __uint128_t disallowed;
        int target;
        int pos;
        int remaining;
    };

    const size_t memoization_limit;
    std::map<int, std::vector<__uint128_t>> precalculated_sums;
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
    std::unordered_set<__uint128_t> seen_masks;
    std::mutex seen_masks_mutex;

    // Add the new private helper method
    PileSetup setup_pile_calculation(const int* data, size_t example, int target_pile_num);

public:
    PileSolver(size_t num_piles, size_t num_cubes, bool do_memoize, bool do_diophantine, const string& tree_path, size_t memoization_limit, const string& memoization_path);
    static constexpr std::array<int, 100> make_sums();
    static constexpr std::array<int, 100> make_cubes();
    void initialize_memoization();
    
    // Core pile manipulation functions
    vector<__uint128_t> make_pile(int target, int remaining, int pos,
                                 __uint128_t pile, __uint128_t disallowed, bool first_level) const;
    int calc_remaining(__uint128_t disallowed) const;
    int sum_pile(__uint128_t pile);

    // Initialization functions
    vector<int> init_distribution();
    vector<int> init_remaining(vector<__uint128_t> piles);
    int init_pos(vector<__uint128_t> piles);

    void build_diophantine_tree(const string& csv_path = "diophantine_small.txt", const string& tree_path = "tree.bin", int max_depth = 40, int min_patterns_leaf=1);
    bool classify_pattern(__uint128_t pile) const;
    #ifdef NB_MODULE
    nb::ndarray<nb::numpy, int, nb::ndim<2>> solve_from_assignment(
        const nb::ndarray<int> assignments,
        int target_pile_num,
        size_t num_threads = 1,
        bool do_mask_dedupe = false);
    #endif
private:
    vector<__uint128_t> find_valid_patterns(int target, __uint128_t disallowed) const;
    bool load_memoization(const std::string& path);
    void save_memoization(const std::string& path);
    bool should_process_mask(__uint128_t mask);
};

#endif //PILES_H
