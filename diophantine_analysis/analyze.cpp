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
        bitset<BITS> pattern;
        bitset<BITS> required_bits;
        bitset<BITS> new_bits;
    };

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

    // Add TreeNode structure
    struct TreeNode {
        bool is_leaf = false;
        int bit_position = -1;  // The bit we're splitting on
        EntropyResult entropy_result;  // Added EntropyResult
        int classification = -1;  // Classification for leaf nodes
        unique_ptr<TreeNode> left;  // Child when bit is 0
        unique_ptr<TreeNode> right; // Child when bit is 1
    };

    unique_ptr<TreeNode> root;  // Root of the decision tree

    unique_ptr<TreeNode> build_decision_tree(vector<int> set_bit_numbers, vector<int> unset_bit_numbers, int depth) {
        cout<<depth<<endl;
        bitset<BITS> set_bits;
        bitset<BITS> unset_bits;
        
        for (int bit : set_bit_numbers) {
            set_bits.set(bit);
        }
        
        for (int bit : unset_bit_numbers) {
            unset_bits.set(bit); 
        }

        auto best_entropy_pattern = find_max_entropy_pattern(1, set_bit_numbers, unset_bit_numbers);
        
        // Create new node
        auto node = make_unique<TreeNode>();

        // If entropy is 0 or we've reached max depth, create leaf node
        if (best_entropy_pattern.entropy == 0 || depth >= 6) {  // Add max depth limit
            node->is_leaf = true;
            node->entropy_result = best_entropy_pattern;
            // Determine classification based on probability
            node->classification = (best_entropy_pattern.probability >= 0.5) ? 1 : 0;
            return node;
        }

        // Find the new bit that was set (there should be exactly one since n=1)
        int split_bit = -1;
        for (size_t i = 0; i < BITS; i++) {
            if (best_entropy_pattern.new_bits[i]) {
                split_bit = i;
                break;
            }
        }

        if (split_bit == -1) {
            // No good split found, create leaf node
            node->is_leaf = true;
            node->entropy_result = best_entropy_pattern;
            node->classification = (best_entropy_pattern.probability >= 0.5) ? 1 : 0;
            return node;
        }

        // Create decision node
        node->is_leaf = false;
        node->bit_position = split_bit;
        node->entropy_result = best_entropy_pattern;

        // Create children
        vector<int> next_set_bits = set_bit_numbers;
        vector<int> next_unset_bits = unset_bit_numbers;

        // Right child (bit is set)
        next_set_bits.push_back(split_bit);
        node->right = build_decision_tree(next_set_bits, next_unset_bits, depth + 1);

        // Left child (bit is not set)
        next_set_bits.pop_back();
        next_unset_bits.push_back(split_bit);
        node->left = build_decision_tree(next_set_bits, next_unset_bits, depth + 1);

        return node;
    }

    // Add helper function for recursive DOT generation
    void tree_to_dot_recursive(std::ostream& out, const TreeNode* node, int& node_count) const {
        if (!node) return;
        
        int current_node = node_count++;
        
        // Node attributes
        if (node->is_leaf) {
            out << "    node" << current_node << " [shape=box,label=\"Class: " 
                << node->classification << "\\nP: " 
                << std::fixed << std::setprecision(2) << node->entropy_result.probability 
                << " (" << node->entropy_result.matched << "/" << node->entropy_result.total << ")"
                << "\\nEntropy: " << (-node->entropy_result.probability * std::log2(node->entropy_result.probability) 
                                    - (1-node->entropy_result.probability) * std::log2(1-node->entropy_result.probability))
                << "\"];\n";
        } else {
            out << "    node" << current_node << " [label=\"Bit " 
                << node->bit_position << "\\nP: " 
                << std::fixed << std::setprecision(2) << node->entropy_result.probability 
                << " (" << node->entropy_result.matched << "/" << node->entropy_result.total << ")"
                << "\\nEntropy: " << (-node->entropy_result.probability * std::log2(node->entropy_result.probability) 
                                    - (1-node->entropy_result.probability) * std::log2(1-node->entropy_result.probability))
                << "\"];\n";
        }
        
        // Recursively process children
        if (node->left) {
            int left_node = node_count;
            out << "    node" << current_node << " -> node" << left_node 
                << " [label=\"0\"];\n";
            tree_to_dot_recursive(out, node->left.get(), node_count);
        }
        
        if (node->right) {
            int right_node = node_count;
            out << "    node" << current_node << " -> node" << right_node 
                << " [label=\"1\"];\n";
            tree_to_dot_recursive(out, node->right.get(), node_count);
        }
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

    EntropyResult calculate_entropy(const bitset<BITS>& pattern, const bitset<BITS>& required_set_bits, const bitset<BITS>& required_unset_bits) const {
        int matched = 0;
        int total = 0;
        EntropyResult result;

        for (const auto& [target, filters] : filters_by_target) {
            for (const auto& filter : filters) {
                // Check both required set and unset bits conditions
                if ((required_set_bits & filter) == required_set_bits && 
                    (required_unset_bits & filter).none()) {
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
        result.pattern = pattern;
        result.required_bits = required_set_bits;
        result.new_bits = pattern & ~required_set_bits;
        
        return result;
    }

    EntropyResult find_max_entropy_pattern(size_t n, 
                                          vector<int> required_set,
                                          vector<int> required_not_set) {

        
        EntropyResult best_entropy;
        best_entropy.entropy = 0.0;

        bitset<BITS> required_bits;
        for (int bit : required_set) {
            required_bits.set(bit);
        }
        // Generate all possible n-bit patterns
        auto patterns = generate_n_bit_patterns(n, required_set, required_not_set);
        
        double max_entropy = -1.0;
        // cout<<patterns.size()<<" possible patterns"<<endl;
        // Find pattern with maximum entropy
        #pragma omp parallel
        {
            EntropyResult local_best_entropy;
            local_best_entropy.entropy = 0;
            
            #pragma omp for schedule(dynamic)
            for (size_t p = 0; p < patterns.size(); p++) {
                const auto& pattern = patterns[p];

                bitset<BITS> required_unset_bits;
                for (int bit : required_not_set) {
                    required_unset_bits.set(bit);
                }
                EntropyResult entropy = calculate_entropy(pattern, required_bits, required_unset_bits);
                if (entropy.entropy > local_best_entropy.entropy) {
                    #pragma omp critical
                    {
                        if (entropy.entropy > best_entropy.entropy) {
                            best_entropy = entropy;
                        }
                    }
                }
            }
        }
        return best_entropy;
    }

    // Add method to start tree construction
    void construct_decision_tree() {
        vector<int> set_bits;
        vector<int> unset_bits;
        root = build_decision_tree(set_bits, unset_bits, 0);
    }

    // Add method to classify new patterns
    bool classify(const bitset<BITS>& pattern) const {
        if (!root) return false;
        
        TreeNode* current = root.get();
        while (!current->is_leaf) {
            if (pattern[current->bit_position]) {
                current = current->right.get();
            } else {
                current = current->left.get();
            }
        }
        return current->classification == 1;
    }

    // Print tree in DOT format to terminal
    void print_tree_dot() const {
        if (!root) return;
        
        // Write DOT header
        cout << "digraph DecisionTree {\n";
        cout << "    node [fontname=\"Arial\"];\n";
        cout << "    edge [fontname=\"Arial\"];\n";
        
        // Generate tree
        int node_count = 0;
        tree_to_dot_recursive(cout, root.get(), node_count);
        
        // Close graph
        cout << "}\n";
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
    filter_tree.construct_decision_tree();
    cout << "Printing decision tree in DOT format:" << endl;
    filter_tree.print_tree_dot();
    // vector<int> set_bits = {5};
    // vector<int> unset_bits = {};
    // auto best_entropy_pattern = filter_tree.find_max_entropy_pattern(1, set_bits, unset_bits);
    
    // cout << "Best entropy pattern results:" << endl;
    // cout << "Entropy: " << best_entropy_pattern.entropy << endl;
    // cout << "Probability: " << best_entropy_pattern.probability << endl;
    // cout << "Matched: " << best_entropy_pattern.matched << endl;
    // cout << "Total: " << best_entropy_pattern.total << endl;
    // cout << "Pattern: " << best_entropy_pattern.pattern << endl;
    // cout << "Required bits: " << best_entropy_pattern.required_bits << endl;


    return 0;
}