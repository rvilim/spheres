#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "src/piles.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <n_piles> <n_cubes>" << endl;
        return 1;
    }

    int n_piles, n_cubes;
    std::istringstream arg1(argv[1]);
    std::istringstream arg2(argv[2]);

    if (!(arg1 >> n_piles)) {
        std::cerr << "Invalid number: " << argv[1] << '\n';
        return 1;
    } else if (!arg1.eof()) {
        std::cerr << "Trailing characters after number: " << argv[1] << '\n';
        return 1;
    }

    if (!(arg2 >> n_cubes)) {
        std::cerr << "Invalid number: " << argv[2] << '\n';
        return 1;
    } else if (!arg2.eof()) {
        std::cerr << "Trailing characters after number: " << argv[2] << '\n';
        return 1;
    }

    // Initialize memoization tables
    initialize_memoization();

    // Get initial distributions and positions
    vector<vector<int>> assigned_piles = init_distribution(n_piles, n_cubes);
    auto start_pos = init_pos(assigned_piles);
    vector<int> assigned_remaining = init_remaining(assigned_piles, n_piles);

    // Check if solution is possible
    if (sums[n_cubes-1] % n_piles != 0) {
        cout << "The sum of the first " << n_cubes << " cubes is not divisible by " 
             << n_piles << ". Not Possible ☹️" << endl;
        return 1;
    }

    // Initialize pile combinations
    vector<vector<vector<int>>> pile_combinations = {{}};
    map<int, int> nums;

    // Iterate through all piles
    for (int pile_idx = 0; pile_idx < n_piles; pile_idx++) {
        cout<<pile_idx<<endl;
        int solutions_count = 0;
        vector<vector<vector<int>>> new_combinations;

        // For each existing combination of piles
        for (const auto& prev_piles : pile_combinations) {
            int target, remaining;
            vector<int> assigned_pile;
            __int128 curr_disallowed = 0;

            if (pile_idx == 0) {
                // First pile uses initial values
                target = assigned_remaining[0];
                remaining = sums[n_cubes-1];
                assigned_pile = assigned_piles[0];
            } else {
                // Subsequent piles use values based on previous piles
                target = assigned_remaining[pile_idx];
                
                // Calculate disallowed positions from previous piles
                for (const auto& prev_pile : prev_piles) {
                    for (int i = 0; i < prev_pile.size(); i++) {
                        if (prev_pile[i]) {
                            curr_disallowed |= (__int128(1) << i);
                        }
                    }
                }
                
                remaining = calc_remaining(curr_disallowed, n_cubes);
                assigned_pile = assigned_piles[pile_idx];
            }

            // Generate next pile
            auto next_piles = make_pile(target, remaining, start_pos, 
                                      assigned_pile, curr_disallowed);
            solutions_count += next_piles.size();

            // Add new combinations
            for (const auto& next_pile : next_piles) {
                vector<vector<int>> new_combination = prev_piles;
                new_combination.push_back(next_pile);
                new_combinations.push_back(new_combination);
            }
        }
        
        nums[pile_idx + 1] = solutions_count;
        pile_combinations = new_combinations;
    }

    // Print results as JSON
    cout << "{";
    auto it = nums.begin();
    while (it != nums.end()) {
        cout << "\"" << it->first << "\":" << it->second;
        if (++it != nums.end()) {
            cout << ",";
        }
    }
    cout << "}" << endl;

    return 0;
}