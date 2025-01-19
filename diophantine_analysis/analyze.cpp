#include <vector>
#include <unordered_map>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <algorithm>
#include <bitset>
#include <iostream>
#include <functional>
#include <iomanip>
#include <map>
#include <omp.h>

using namespace std;

const size_t BITS = 128;


class BitFilterTree {

private:

    string cache_path = "diophantine.cache";
    unordered_map<int, vector<bitset<BITS>>> filters_by_target;
    
    // Add EntropyResult struct at class level
    struct EntropyResult {
        double entropy;
        double probability;
        int matched;
        int total;
    };

    // Add decision tree node structure
    struct TreeNode {
        int bit_position = -1;  // -1 indicates leaf node
        bool is_leaf = false;
        int classification = -1;  // Only valid for leaf nodes, -1 means no classification
        double probability = 0.0;
        unique_ptr<TreeNode> left;   // bit is 0
        unique_ptr<TreeNode> right;  // bit is 1
    };
    
    unique_ptr<TreeNode> root;

    bool save_filters_to_cache() {
        std::ofstream cache(cache_path, std::ios::binary);
        if (!cache.is_open()) {
            return false;
        }
        
        size_t num_targets = filters_by_target.size();
        cache.write(reinterpret_cast<const char*>(&num_targets), sizeof(num_targets));
        
        for (const auto& [target, filters] : filters_by_target) {
            cache.write(reinterpret_cast<const char*>(&target), sizeof(target));
            size_t size = filters.size();
            cache.write(reinterpret_cast<const char*>(&size), sizeof(size));
            
            for (const auto& filter : filters) {
                cache.write(reinterpret_cast<const char*>(&filter), sizeof(bitset<BITS>));
            }
        }
        
        return true;
    }
    
    bool load_filters_from_cache() {
        std::ifstream cache(cache_path, std::ios::binary);
        if (!cache.is_open()) {
            return false;
        }
        
        size_t num_targets;
        cache.read(reinterpret_cast<char*>(&num_targets), sizeof(num_targets));
        
        filters_by_target.clear();
        
        for (size_t i = 0; i < num_targets; i++) {
            int target;
            size_t size;
            cache.read(reinterpret_cast<char*>(&target), sizeof(target));
            cache.read(reinterpret_cast<char*>(&size), sizeof(size));
            
            vector<bitset<BITS>> filters;
            filters.reserve(size);
            
            for (size_t j = 0; j < size; j++) {
                bitset<BITS> filter;
                cache.read(reinterpret_cast<char*>(&filter), sizeof(bitset<BITS>));
                filters.push_back(filter);
            }
            
            filters_by_target[target] = std::move(filters);
        }
        
        return true;
    }

    void generate_patterns_recursive(
        vector<bitset<BITS>>& patterns,
        bitset<BITS> current_pattern,
        const vector<int>& available_positions,
        size_t start_idx,
        size_t bits_remaining
    ) {
        if (bits_remaining == 0) {
            patterns.push_back(current_pattern);
            return;
        }
        
        // Not enough positions left to set required bits
        if (available_positions.size() - start_idx < bits_remaining) {
            return;
        }
        
        for (size_t i = start_idx; i <= available_positions.size() - bits_remaining; i++) {
            auto new_pattern = current_pattern;
            new_pattern.set(available_positions[i]);
            generate_patterns_recursive(patterns, new_pattern, available_positions, i + 1, bits_remaining - 1);
        }
    }




    unique_ptr<TreeNode> build_tree_recursive(
        const vector<bitset<BITS>>& patterns,
        const vector<int>& classifications,
        bitset<BITS> current_pattern,
        vector<bool> available_bits,
        int depth = 0
    ) {
        // Print progress information
        cout << string(depth * 2, ' ') << "Building tree node at depth " << depth 
             << " with " << patterns.size() << " patterns" << endl;
        
        auto node = make_unique<TreeNode>();
        
        // Create leaf node if we've reached depth 2 or if all patterns have same classification
        if (depth >= 2 || patterns.empty()) {
            node->is_leaf = true;
            // Find majority class
            map<int, int> class_counts;
            for (int c : classifications) class_counts[c]++;
            
            int max_count = 0;
            for (const auto& [c, count] : class_counts) {
                if (count > max_count) {
                    max_count = count;
                    node->classification = c;
                }
            }
            node->probability = classifications.empty() ? 0.0 : 
                static_cast<double>(max_count) / classifications.size();
            return node;
        }
        
        // Find best bit to split on using entropy - parallelize this section
        int best_bit = -1;
        double best_entropy = -1;
        
        #pragma omp parallel
        {
            int local_best_bit = -1;
            double local_best_entropy = -1;
            
            #pragma omp for schedule(dynamic)
            for (size_t bit = 0; bit < BITS; bit++) {
                if (!available_bits[bit]) continue;
                
                bitset<BITS> test_pattern = current_pattern;
                test_pattern.set(bit);
                EntropyResult entropy = calculate_entropy(test_pattern, current_pattern);
                
                if (entropy.entropy > local_best_entropy) {
                    local_best_entropy = entropy.entropy;
                    local_best_bit = bit;
                }
            }
            
            #pragma omp critical
            {
                if (local_best_entropy > best_entropy) {
                    best_entropy = local_best_entropy;
                    best_bit = local_best_bit;
                }
            }
        }
        
        // If no good split found, create leaf with majority class
        if (best_bit == -1) {
            node->is_leaf = true;
            map<int, int> class_counts;
            for (int c : classifications) class_counts[c]++;
            
            int max_count = 0;
            for (const auto& [c, count] : class_counts) {
                if (count > max_count) {
                    max_count = count;
                    node->classification = c;
                }
            }
            node->probability = static_cast<double>(max_count) / classifications.size();
            return node;
        }
        
        // Split patterns based on bit
        vector<bitset<BITS>> left_patterns, right_patterns;
        vector<int> left_classes, right_classes;
        
        for (size_t i = 0; i < patterns.size(); i++) {
            if (patterns[i][best_bit]) {
                right_patterns.push_back(patterns[i]);
                right_classes.push_back(classifications[i]);
            } else {
                left_patterns.push_back(patterns[i]);
                left_classes.push_back(classifications[i]);
            }
        }
        
        // Create decision node
        node->bit_position = best_bit;
        available_bits[best_bit] = false;
        
        // Recursively build children
        node->left = build_tree_recursive(left_patterns, left_classes, current_pattern, available_bits, depth + 1);
        current_pattern.set(best_bit);
        node->right = build_tree_recursive(right_patterns, right_classes, current_pattern, available_bits, depth + 1);
        
        // Print when returning from important nodes
        if (depth < 3) {
            cout << string(depth * 2, ' ') << "Completed node at depth " << depth 
                 << " (bit: " << best_bit << ")" << endl;
        }
        
        return node;
    }

public:

    const vector<bitset<BITS>>& get_filters_for_target(int target) const {
        static const vector<bitset<BITS>> empty_vector;
        auto it = filters_by_target.find(target);
        return it != filters_by_target.end() ? it->second : empty_vector;
    }

    double entropy_from_filter(bitset<BITS> pattern) {
        int matches = 0;
        int total = 0;
        
        // Count matches and total
        for (const auto& [target, filters] : filters_by_target) {
            for (const auto& filter : filters) {
                if ((pattern & filter) == pattern) {
                    matches++;
                }
                total++;
            }
        }
        
        // Calculate probabilities
        double p_match = static_cast<double>(matches) / total;
        double p_nomatch = static_cast<double>(total - matches) / total;
        
        // Calculate entropy (-p(x)log₂(p(x)) for each outcome)
        double entropy = 0.0;
        if (p_match > 0) {
            entropy -= p_match * std::log2(p_match);
        }
        if (p_nomatch > 0) {
            entropy -= p_nomatch * std::log2(p_nomatch);
        }
        
        return entropy;
    }

    bool read_filters_from_csv(const string& filename) {
        if (load_filters_from_cache()) {
            cout << "Loaded filters from cache" << endl;
            return true;
        }
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open file: " << filename << endl;
            return false;
        }

        filters_by_target.clear();
        string line;

        while (std::getline(file, line)) {
            stringstream ss(line);
            string value;
            vector<int> row;
            
            try {
                while (std::getline(ss, value, ',')) {
                    int pos = std::stoi(value);
                    if (pos <= 0 || pos > BITS) {
                        continue;
                    }
                    row.push_back(pos);
                }
                
                if (!row.empty()) {
                    int target = row.back();  // Get target value
                    row.pop_back();  // Remove target from positions

                    bitset<BITS> filter;  // Initialize empty bitset
                    for (int pos : row) {
                        filter.set(pos-1);  // Set bit at position (1-based indexing)
                    }
                    filters_by_target[target].push_back(filter);  // Store filter in appropriate target vector
                }
            } catch (const std::exception& e) {
                cerr << "Error parsing line: " << line << endl;
                cerr << "Exception: " << e.what() << endl;
            }
        }
        
        if (!filters_by_target.empty()) {
            if (save_filters_to_cache()) {
                cout << "Saved filters to cache" << endl;
            } else {
                cerr << "Failed to save cache file" << endl;
            }
        }
        
        return !filters_by_target.empty();
    }

    vector<bitset<BITS>> generate_n_bit_patterns(
        size_t n,
        const vector<int>& must_set,
        const vector<int>& must_not_set
    ) {
        vector<bitset<BITS>> patterns;
        bitset<BITS> base_pattern;
        
        // Set required bits
        for (int bit : must_set) {
            base_pattern.set(bit);
        }
        
        // Get available positions (excluding must_set and must_not_set bits)
        vector<int> available_positions;
        for (size_t i = 0; i < BITS; i++) {
            if (std::find(must_set.begin(), must_set.end(), i) == must_set.end() &&
                std::find(must_not_set.begin(), must_not_set.end(), i) == must_not_set.end()) {
                available_positions.push_back(i);
            }
        }
        
        // Generate combinations using recursive helper
        generate_patterns_recursive(patterns, base_pattern, available_positions, 0, n);
        return patterns;
    }

    EntropyResult calculate_entropy(const bitset<BITS>& pattern, const bitset<BITS>& required_bits) const {
        int matched = 0;
        int total = 0;
        EntropyResult result;

        // Second pass: calculate entropy directly
        for (const auto& [target, filters] : filters_by_target) {
            for (const auto& filter : filters) {
                if ((required_bits & filter) == required_bits) {
                    if ((pattern & filter) == pattern) {
                        matched += 1;
                    }
                    total += 1;
                }
            }
        }

        double p = static_cast<double>(matched) / total;
        
        // Handle edge cases where p is 0 or 1 to avoid log(0)
        if (total == 0 || p == 0.0 || p == 1.0) {
            result.entropy = 0.0;  // No uncertainty when p is 0 or 1
        } else {
            // Calculate binary entropy: H(p) = -p*log2(p) - (1-p)*log2(1-p)
            result.entropy = -p * std::log2(p) - (1-p) * std::log2(1-p);
        }
        
        result.probability = p;
        result.matched = matched;
        result.total = total;
        return result;
    }

    bitset<BITS> find_max_entropy_pattern(size_t n, 
                                          vector<int> required_set,
                                          vector<int> required_not_set) {

        
        bitset<BITS> required_bits;
        for (int bit : required_set) {
            required_bits.set(bit);
        }
        // Generate all possible n-bit patterns
        auto patterns = generate_n_bit_patterns(n, required_set, required_not_set);
        
        bitset<BITS> best_pattern;
        double max_entropy = -1.0;
        cout<<patterns.size()<<" possible patterns"<<endl;
        // Find pattern with maximum entropy
        #pragma omp parallel
        {
            bitset<BITS> local_best_pattern;
            double local_max_entropy = -1.0;
            
            #pragma omp for schedule(dynamic)
            for (size_t p = 0; p < patterns.size(); p++) {
                const auto& pattern = patterns[p];

                EntropyResult entropy = calculate_entropy(pattern, required_bits);
                if (entropy.entropy > local_max_entropy) {
                    local_max_entropy = entropy.entropy;
                    local_best_pattern = pattern;
                    #pragma omp critical
                    {
                        if (entropy.entropy > max_entropy) {
                            max_entropy = entropy.entropy;
                            best_pattern = pattern;
                            cout << "New max entropy: " << entropy.entropy << " with pattern: " << pattern << endl;
                            cout << "Probability: " << entropy.probability << endl;
                            cout << "Matches: " << entropy.matched << " / " << entropy.total << endl;
                        }
                    }
                }
            }
        }
        return best_pattern;
    }


private:

};

int main() {
    BitFilterTree filter_tree;
    
    cout << "Reading filters" << endl;
    if (!filter_tree.read_filters_from_csv("../diophantine.txt")) {
        cerr << "Failed to read filters" << endl;
        return 1;
    }

    vector<int> set_bits = {5};
    vector<int> unset_bits = {};
    filter_tree.find_max_entropy_pattern(1, set_bits, unset_bits);

    return 0;
}