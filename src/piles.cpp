#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include <cstdlib>
#include <map>
#include <fstream>
#include "bitfiltertree.h"
#include <mutex>
#include <atomic>

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
                       size_t memoization_limit,
                       const std::string& memoization_path)
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

    if (enable_memoize) {
        if (!load_memoization(memoization_path)) {
            initialize_memoization();
            save_memoization(memoization_path);
        }
    }
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

vector<int> PileSolver::init_distribution() {
    vector<int> assignments(n_cubes, -1);  // Initialize all positions as unassigned (-1)
    int target = sums[n_cubes-1]/n_piles;

    // Assign highest cube to pile 0
    assignments[n_cubes-1] = 0;

    // Assign cubes to other piles if needed
    for (int pile_num = 1; pile_num < n_piles; pile_num++) {
        if (cubes[n_cubes-pile_num]+cubes[n_cubes-pile_num-1] > target) {
            assignments[n_cubes-pile_num-1] = pile_num;
        }
    }

    return assignments;
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

vector<vector<int>> PileSolver::solve_from_assignment(const nb::ndarray<int> assignments,
                                     int target_pile_num,
                                     size_t num_threads) {
    // Get raw pointer to numpy array data and dimensions
    const int* data = assignments.data();
    size_t num_examples = assignments.shape(0);
    // size_t size = assignments.shape(1);

    // Vector to store all solutions across all examples
    std::vector<std::vector<int>> all_assignments;
    std::mutex all_assignments_mutex;  // Protect access to all_assignments
    
    // Create thread pool
    std::vector<std::thread> threads;
    std::atomic<size_t> next_example{0};
    
    // Launch worker threads
    const size_t chunk_size = 100;
    for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
        threads.emplace_back([&, thread_id]() {
            while (true) {
                // Get next chunk of examples to process
                size_t chunk_start = next_example.fetch_add(chunk_size);
                if (chunk_start >= num_examples) {
                    break;
                }
                
                // Calculate actual chunk end (handling the last chunk properly)
                size_t chunk_end = std::min(chunk_start + chunk_size, num_examples);
                
                // Process all examples in this chunk
                for (size_t example = chunk_start; example < chunk_end; example++) {
                    // Initialize target pile and disallowed mask for this example
                    __uint128_t target_pile = 0;
                    __uint128_t disallowed = 0;
                    
                    // Process assignments to build disallowed bits
                    for (size_t i = 0; i < n_cubes; i++) {
                        int current_assignment = data[example * n_cubes + i];
                        if (current_assignment != -1) {
                            if (current_assignment == target_pile_num) {
                                BitFilterTree::SetBit(target_pile, i);
                            } else {
                                disallowed |= (__uint128_t(1) << i);
                            }
                        }
                    }
                    
                    // Calculate target sum for the pile we're solving
                    int target = sums[n_cubes-1]/n_piles - sum_pile(target_pile);
                    
                    // Find highest available position
                    int pos = n_cubes - 1;
                    while (pos >= 0 && (disallowed & (__uint128_t(1) << pos))) {
                        pos--;
                    }
                    
                    // Calculate remaining total
                    int remaining = calc_remaining(disallowed);

                    // Make the pile and combine with initial assignments
                    auto solutions = make_pile(target, remaining, pos, target_pile, disallowed);
                    
                    // Thread-safe addition of solutions
                    if (!solutions.empty()) {
                        std::lock_guard<std::mutex> lock(all_assignments_mutex);
                        for (const auto& solution : solutions) {
                            std::vector<int> final_assignments(n_cubes);
                            for (size_t i = 0; i < n_cubes; i++) {
                                final_assignments[i] = data[example * n_cubes + i];
                            }
                            
                            for (size_t i = 0; i < n_cubes; i++) {
                                if (solution & (__uint128_t(1) << i)) {
                                    final_assignments[i] = target_pile_num;
                                }
                            }
                            
                            all_assignments.push_back(final_assignments);
                        }
                    }
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    cout<<"All joined"<<endl;
    return all_assignments;
}

bool PileSolver::load_memoization(const std::string& path) {
    if (path.empty()) return false;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.good()) return false;
    
    auto start = std::chrono::high_resolution_clock::now();
    try {
        size_t map_size;
        file.read(reinterpret_cast<char*>(&map_size), sizeof(size_t));
        
        for (size_t i = 0; i < map_size; i++) {
            int sum;
            size_t vec_size;
            file.read(reinterpret_cast<char*>(&sum), sizeof(int));
            file.read(reinterpret_cast<char*>(&vec_size), sizeof(size_t));
            
            std::vector<__int128> values(vec_size);
            file.read(reinterpret_cast<char*>(values.data()), vec_size * sizeof(__int128));
            precalculated_sums[sum] = std::move(values);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Loaded memoization from file in " << duration.count() << " ms" << std::endl;
        return true;
    } catch (...) {
        std::cerr << "Error loading memoization file" << std::endl;
        return false;
    }
}

void PileSolver::save_memoization(const std::string& path) {
    if (path.empty()) return;
    
    std::ofstream file(path, std::ios::binary);
    if (!file.good()) {
        std::cerr << "Error opening memoization file for writing" << std::endl;
        return;
    }
    
    try {
        size_t map_size = precalculated_sums.size();
        file.write(reinterpret_cast<const char*>(&map_size), sizeof(size_t));
        
        for (const auto& [sum, values] : precalculated_sums) {
            size_t vec_size = values.size();
            file.write(reinterpret_cast<const char*>(&sum), sizeof(int));
            file.write(reinterpret_cast<const char*>(&vec_size), sizeof(size_t));
            file.write(reinterpret_cast<const char*>(values.data()), vec_size * sizeof(__int128));
        }
        std::cout << "Saved memoization to file" << std::endl;
    } catch (...) {
        std::cerr << "Error saving memoization file" << std::endl;
    }
}

NB_MODULE(piles, m) {
    nb::class_<PileSolver>(m, "PileSolver")
        .def(nb::init<size_t, size_t, bool, bool, const std::string&, size_t, const std::string&>(),
             nb::arg("num_piles"),
             nb::arg("num_cubes"),
             nb::arg("do_memoize") = true,
             nb::arg("do_diophantine") = true,
             nb::arg("tree_path") = "tree.bin",
             nb::arg("memoization_limit") = 26,
             nb::arg("memoization_path") = "")
        .def("init_pos", &PileSolver::init_pos)
        .def("init_distribution", &PileSolver::init_distribution)
        .def("init_remaining", &PileSolver::init_remaining)
        .def("calc_remaining", [](PileSolver& self, __int128 disallowed) -> int {
            return self.calc_remaining(disallowed);
        })
        .def("make_pile", [](PileSolver& self, int target, int remaining, int pos, 
                            __uint128_t pile, __int128 disallowed) -> std::vector<__uint128_t> {
            return self.make_pile(target, remaining, pos, pile, disallowed);
        })
        .def("classify_pattern", [](PileSolver& self, __uint128_t pile) -> bool {
            return self.classify_pattern(pile);
        })
        .def("solve_from_assignment", &PileSolver::solve_from_assignment,
             nb::arg("assignments"),
             nb::arg("target_pile_num"),
             nb::arg("num_threads") = 1,
             "Solve for a pile given existing assignments. Returns a list of possible assignments, where each assignment is a numpy array and -1 indicates unassigned")
        .def("initialize_memoization", &PileSolver::initialize_memoization, 
              "Initialize the memoization table for faster lookups of small positions");

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

    m.def("process_u128_list", [](const std::vector<__uint128_t> &vals) {
        // do something with vals
        return vals.size(); // for example
    });
#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
