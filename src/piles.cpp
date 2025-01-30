#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "piles.h"
#include <cstdlib>
#include <map>
#include <fstream>
#include "bitfiltertree.h"

#ifdef WITH_PYTHON
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#endif

// auto n_cubes=23;
// auto n_piles=3;
using namespace std;

// const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
// const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

PileSolver::PileSolver() : sums(make_sums()), cubes(make_cubes()) {
    // initialize_memoization();
    // Initialize and load the bit filter tree
    auto start = std::chrono::high_resolution_clock::now();
    
    filter_tree = std::make_unique<BitFilterTree>();
    auto loaded_tree = filter_tree->LoadTreeBinary("tree.bin");
    if (!loaded_tree) {
        std::cerr << "Failed to load bit filter tree from tree.bin" << std::endl;
    } else {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Successfully loaded bit filter tree in " << duration.count() << " ms" << std::endl;
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
    for (__int128 bits = 0; bits < (1 << MEMOIZATION_LIMIT); bits++) {
        int sum = 0;
        // Calculate sum for this combination
        for (int pos = 0; pos < MEMOIZATION_LIMIT; pos++) {
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

int PileSolver::sum_pile(vector<int> pile) {
    int s=0;
    for(int pos=0; pos<pile.size(); pos++){
        s+=pile[pos]*cubes[pos];
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

bool PileSolver::classify_pattern(const vector<int>& pile) const {
    if (!filter_tree) return false;
    
    // Convert pile vector to __uint128_t
    __uint128_t set_bits = 0;
    for (size_t i = 0; i < pile.size(); i++) {
        if (pile[i] == 1) {
            set_bits |= ((__uint128_t)1 << i);
        }
    }
    return filter_tree->ClassifyPattern(set_bits, pile.size());
}

vector<vector<int>> PileSolver::make_pile(int target, int remaining, int pos,
                                          vector<int> &pile, __int128 disallowed) {
    vector<vector<int>> solutions;

    if ((pos < MEMOIZATION_LIMIT) && pile.size()>=MEMOIZATION_LIMIT) {
        auto valid_patterns = find_valid_patterns(target, disallowed);        
        
        for (const auto& bits : valid_patterns) {
            vector<int> new_solution = pile;
            for (int i = 0; i <= pos; i++) {
                new_solution[i] = (bits & ((__int128)1 << i)) ? 1 : 0;
            }
            
            // Convert pile directly to __uint128_t for classification
            if (!filter_tree || !classify_pattern(new_solution)) {
                solutions.push_back(new_solution);
            }
        }
        return solutions;
    }

    // If current position is disallowed, just move down one position
    if (disallowed & ((__int128)1 << pos)) {
        if (pos==0) return solutions;
        return make_pile(target, remaining, pos - 1, pile, disallowed);
    }

    if (target > remaining) {
        return solutions;
    }

    if (cubes[pos] == target) {
        pile[pos] = true;
        if (!filter_tree || !classify_pattern(pile)) {
            solutions.push_back(pile);
        }
        pile[pos] = false;
    }

    if (pos == 0) return solutions;

    // Try setting the current position
    if (target - cubes[pos] > 0) {
        pile[pos] = true;
        auto sub_solutions = make_pile(target - cubes[pos], remaining - cubes[pos], pos - 1, pile, disallowed);
        solutions.insert(solutions.end(), sub_solutions.begin(), sub_solutions.end());
        pile[pos] = false;
    }

    // Try without setting the current position
    auto more_solutions = make_pile(target, remaining - cubes[pos], pos - 1, pile, disallowed);
    solutions.insert(solutions.end(), more_solutions.begin(), more_solutions.end());

    return solutions;
}

vector<vector<int>> PileSolver::init_distribution(int n_piles, int n_cubes) {
    int target = sums[n_cubes-1]/n_piles;

    vector<vector<int>> piles(n_piles, vector<int>(n_cubes, false));
    piles[0][n_cubes-1]=true;

    for (int pile_num=1; pile_num<n_piles; pile_num++){
        // The condition here is that we can place things without loss of generality
        // as long as two adjacent cubes add up to greater than the target. As soon
        // as that's not true the smaller could either go in a new pile, or double up
        // with an already placed pile, and we have to leave it to the main solver to
        // figure tha one out.
        if (cubes[n_cubes-pile_num]+cubes[n_cubes-pile_num-1]>target){
            piles[pile_num][n_cubes-pile_num-1]=true;
        }
    }

    return piles;
}

vector<int> PileSolver::init_remaining(vector<vector<int>> piles, int n_piles) {
    vector<int> remaining={};
    auto n_cubes = piles[0].size();
    for(auto & pile : piles){
        remaining.emplace_back(sums[n_cubes - 1] / n_piles - sum_pile(pile));
    }

    return remaining;
}

int PileSolver::init_pos(vector<vector<int>> piles) {
    for (int i=piles.size()-1; i>=0;i--){
        auto pile = piles[i];

        for(int j=0; j<pile.size(); j++){
            if (pile[j]){
                return j-1;
            }
        }
    }
    cout<<"yikes init_pos"<<endl;
    return -1;
}

int PileSolver::calc_remaining(__int128 disallowed, int n_cubes) {
    int remaining = sums[n_cubes-1];

    for (int pos = 0; pos < n_cubes; pos++) {
        if (disallowed & ((__int128)1 << pos)) {
            remaining -= cubes[pos];
        }
    }
    return remaining;
}

void PileSolver::build_diophantine_tree(const string& csv_path, const string& tree_path, int max_depth) {
    cout << "Reading filters from " << csv_path << endl;
    if (!filter_tree->ReadFiltersFromCsv(csv_path)) {
        cerr << "Failed to read filters" << endl;
        return;
    }

    cout << "Building Tree from file " << csv_path << endl;
    // BuildTree returns bool, not a unique_ptr
    if (!filter_tree->BuildTree(FilterPattern(), max_depth, 1)) {
        cerr << "Failed to build tree" << endl;
        return;
    }
    
    // Save the tree
    cout << "Saving tree to tree.bin" << endl;
    if (!filter_tree->SaveTreeBinary(tree_path)) {
        cerr << "Failed to save tree" << endl;
    }
    auto dot_tree = filter_tree->CreateDotTree();
    std::ofstream dot_file("tree.dot");
    if (dot_file.is_open()) {
        dot_file << dot_tree;
        dot_file.close();
    }
}

#ifdef WITH_PYTHON
namespace py = pybind11;

PYBIND11_MODULE(piles, m) {
    py::class_<PileSolver>(m, "PileSolver")
        .def(py::init<>())
        .def("init_pos", &PileSolver::init_pos)
        .def("init_distribution", &PileSolver::init_distribution)
        .def("init_remaining", &PileSolver::init_remaining)
        .def("calc_remaining", &PileSolver::calc_remaining)
        .def("make_pile", [](PileSolver& self, int target, int remaining, int pos, 
                            std::vector<int>& pile, int64_t disallowed) {
            return self.make_pile(target, remaining, pos, pile, (__int128)disallowed);
        })
        .def("build_diophantine_tree", &PileSolver::build_diophantine_tree,
             py::arg("csv_path") = "diophantine.txt",
             py::arg("tree_path") = "tree.bin",
             py::arg("max_depth") = 40)
        .def("classify_pattern", [](PileSolver& self, const std::vector<int>& pile) {
            return self.classify_pattern(pile);
        });

    m.doc() = R"pbdoc(
        Pybind11 piles
        -----------------------

        .. currentmodule:: piles

        .. autosummary::
           :toctree: _generate

           init_pos
           init_distribution
           make_pile
    )pbdoc";

    m.def("initialize_memoization", &PileSolver::initialize_memoization, R"pbdoc(
        Initialize the memoization table for faster lookups of small positions
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
#endif
