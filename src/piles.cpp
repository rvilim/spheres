#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include <cstdlib>
#include <map>
#include <fstream>
#include "bitfiltertree.h"

#include <nanobind/nanobind.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string.h>
#include <cstdint> // for __uint128_t

namespace nb = nanobind;  // Add this line to use nb:: shorthand

// Now we can specialize it
namespace nanobind::detail {

template <>
struct dtype_traits<__uint128_t> {
    static constexpr dlpack::dtype value {
        (uint8_t) dlpack::dtype_code::UInt,
        128,
        1
    };
    static constexpr auto name = const_name("uint128");
};

} // end namespace nanobind::detail

using namespace std;


// Rest of your code...
PileSolver::PileSolver(size_t num_piles, 
                       size_t num_cubes, 
                       bool do_memoize,
                       bool do_diophantine,
                       const std::string& tree_path, 
                       size_t memoization_limit) 
    : memoization_limit(memoization_limit),
      sums(make_sums()),
      cubes(make_cubes()),
      enable_memoize(do_memoize),
      enable_diophantine(do_diophantine),
      n_cubes(num_cubes),
      n_piles(num_piles) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if (enable_diophantine) {
        filter_tree = std::make_unique<BitFilterTree>(num_cubes);
        auto loaded_tree = filter_tree->LoadTreeBinary(tree_path);
        if (!loaded_tree) {
            std::cerr << "Failed to load bit filter tree from " << tree_path << std::endl;
        } else {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "Successfully loaded bit filter tree in " << duration.count() << " ms" << std::endl;
        }
    }

    if (enable_memoize) initialize_memoization();
}

constexpr std::array<int, 100> PileSolver::make_sums() {
    std::array<int, 100> arr = {};
    for (int n = 1; n <= 100; n++) {
        arr[n-1] = ((n * (n + 1)) / 2) * ((n * (n + 1)) / 2);
    }
    return arr;
}

constexpr std::array<int, 100> PileSolver::make_cubes() {
    std::array<int, 100> arr = {};
    for (int i = 1; i <= 100; i++) {
        arr[i-1] = i * i * i;
    }
    return arr;
}

void PileSolver::initialize_memoization() {
    auto start = std::chrono::high_resolution_clock::now();
    // Try all possible combinations of the first MEMOIZATION_LIMIT bits
    for (__int128 bits = 0; bits < (1 << memoization_limit); bits++) {
        int sum = 0;
        // Calculate sum for this combination
        for (int pos = 0; pos < memoization_limit; pos++) {
            if (bits & ((__int128)1 << pos)) {
                sum += cubes[pos];
            }
        }
        precalculated_sums[sum].push_back(bits);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Memoization initialization took " << duration.count() << " ms" << std::endl;
}

int PileSolver::sum_pile(__uint128_t pile) {
    int s = 0;
    for(size_t pos = 0; pos < n_cubes; pos++) {
        if (BitFilterTree::IsBitSet(pile, pos)) {
            s += cubes[pos];
        }
    }
    return s;
}

vector<__int128> PileSolver::find_valid_patterns(int target, __int128 disallowed) {
    vector<__int128> valid_patterns;
    
    auto it = precalculated_sums.find(target);
    if (it == precalculated_sums.end()) {
        return valid_patterns;
    }

    // Check all patterns for this sum
    const auto& patterns = it->second;
    for (const auto& bits : patterns) {
        if ((bits & disallowed) == 0) {
            valid_patterns.push_back(bits);
        }
    }
    return valid_patterns;
}

bool PileSolver::classify_pattern(__uint128_t pile) const {
    if (!filter_tree) return false;
    return filter_tree->ClassifyPattern(pile);
}

vector<__uint128_t> PileSolver::make_pile(int target, int remaining, int pos,
                                       __uint128_t pile, __int128 disallowed) {
    vector<__uint128_t> solutions;

    if (enable_memoize && (pos == memoization_limit - 1) && n_cubes >= memoization_limit) {
        auto valid_patterns = find_valid_patterns(target, disallowed);        
        
        for (const auto& bits : valid_patterns) {
            __uint128_t new_solution = pile;
            for (int i = 0; i < memoization_limit; i++) {
                if (bits & ((__int128)1 << i)) {
                    BitFilterTree::SetBit(new_solution, i);
                }
            }
            
            if (!enable_diophantine || (enable_diophantine && !classify_pattern(new_solution))) {
                solutions.push_back(new_solution);
            }
        }
        return solutions;
    }

    // If current position is disallowed, just move down one position
    if (disallowed & ((__int128)1 << pos)) {
        if (pos == 0) return solutions;
        return make_pile(target, remaining, pos - 1, pile, disallowed);
    }

    if (target > remaining) {
        return solutions;
    }

    if (cubes[pos] == target) {
        BitFilterTree::SetBit(pile, pos);
        if (!enable_diophantine || (enable_diophantine && !classify_pattern(pile))) {
            solutions.push_back(pile);
        }
        pile &= ~(__uint128_t(1) << pos); // Clear the bit
    }

    if (pos == 0) return solutions;

    // Try setting the current position
    if (target - cubes[pos] > 0) {
        BitFilterTree::SetBit(pile, pos);
        auto sub_solutions = make_pile(target - cubes[pos], remaining - cubes[pos], pos - 1, pile, disallowed);
        solutions.insert(solutions.end(), sub_solutions.begin(), sub_solutions.end());
        pile &= ~(__uint128_t(1) << pos); // Clear the bit
    }

    // Try without setting the current position
    auto more_solutions = make_pile(target, remaining - cubes[pos], pos - 1, pile, disallowed);
    solutions.insert(solutions.end(), more_solutions.begin(), more_solutions.end());

    return solutions;
}

vector<__uint128_t> PileSolver::init_distribution() {
    vector<__uint128_t> piles(n_piles, 0);
    int target = sums[n_cubes-1]/n_piles;

    BitFilterTree::SetBit(piles[0], n_cubes-1);

    for (int pile_num = 1; pile_num < n_piles; pile_num++) {
        if (cubes[n_cubes-pile_num]+cubes[n_cubes-pile_num-1] > target) {
            BitFilterTree::SetBit(piles[pile_num], n_cubes-pile_num-1);
        }
    }

    return piles;
}

vector<int> PileSolver::init_remaining(vector<__uint128_t> piles) {
    vector<int> remaining;
    for(auto pile : piles) {
        remaining.emplace_back(sums[n_cubes - 1] / n_piles - sum_pile(pile));
    }
    return remaining;
}

int PileSolver::init_pos(vector<__uint128_t> piles) {
    for (int i = n_piles-1; i >= 0; i--) {
        auto pile = piles[i];
        for(int j = 0; j < n_cubes; j++) {
            if (BitFilterTree::IsBitSet(pile, j)) {
                return j-1;
            }
        }
    }
    cout << "yikes init_pos" << endl;
    return -1;
}

int PileSolver::calc_remaining(__int128 disallowed) {
    int remaining = sums[n_cubes-1];

    for (int pos = 0; pos < n_cubes; pos++) {
        if (disallowed & ((__int128)1 << pos)) {
            remaining -= cubes[pos];
        }
    }
    return remaining;
}

// void PileSolver::build_diophantine_tree(const string& csv_path, const string& tree_path, int max_depth, int min_patterns_leaf) {
//     cout << "Reading filters from " << csv_path << endl;
//     // Create a default cache path based on the csv path
//     string cache_path = csv_path + ".cache";
//     if (!filter_tree->ReadFiltersFromCsv(csv_path, cache_path)) {
//         cerr << "Failed to read filters" << endl;
//         return;
//     }

//     cout << "Building Tree from file " << csv_path << endl;
//     // BuildTree returns bool, not a unique_ptr
//     if (!filter_tree->BuildTree(FilterPattern(), max_depth, min_patterns_leaf)) {
//         cerr << "Failed to build tree" << endl;
//         return;
//     }
    
//     // Save the tree
//     cout << "Saving tree to tree.bin" << endl;
//     if (!filter_tree->SaveTreeBinary(tree_path)) {
//         cerr << "Failed to save tree" << endl;
//     }
//     auto dot_tree = filter_tree->CreateDotTree();
//     std::ofstream dot_file("tree.dot");
//     if (dot_file.is_open()) {
//         dot_file << dot_tree;
//         dot_file.close();
//     }
// }

NB_MODULE(piles, m) {
    nb::class_<PileSolver>(m, "PileSolver")
        .def(nb::init<size_t, size_t, bool, bool, const std::string&, size_t>(),
             nb::arg("num_piles"),
             nb::arg("num_cubes"),
             nb::arg("do_memoize") = true,
             nb::arg("do_diophantine") = true,
             nb::arg("tree_path") = "tree.bin",
             nb::arg("memoization_limit") = 26)
        .def("init_pos", &PileSolver::init_pos)
        .def("init_distribution", &PileSolver::init_distribution)
        .def("init_remaining", &PileSolver::init_remaining)
        .def("calc_remaining", &PileSolver::calc_remaining)
        .def("make_pile", [](PileSolver& self, int target, int remaining, int pos, 
                            __uint128_t pile, __int128 disallowed) {
            return self.make_pile(target, remaining, pos, pile, disallowed);
        })
        .def("classify_pattern", &PileSolver::classify_pattern);

    m.doc() = R"pbdoc(
        Nanobind piles
        -----------------------

        .. currentmodule:: piles

        .. autosummary::
           :toctree: _generate

           init_pos
           init_distribution
           make_pile
    )pbdoc";

    m.def("initialize_memoization", &PileSolver::initialize_memoization, 
          "Initialize the memoization table for faster lookups of small positions");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
