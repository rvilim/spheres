#include "bitfiltertree.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <omp.h>
#include <functional>

using namespace std;



bool BitFilterTree::SaveFiltersToCache() {
    std::ofstream cache(kCachePath, std::ios::binary);
    if (!cache.is_open()) {
        return false;
    }
    
    size_t num_patterns = patterns_.size();
    cache.write(reinterpret_cast<const char*>(&num_patterns), sizeof(num_patterns));
    
    for (const auto& pattern : patterns_) {
        size_t req_size = pattern.required.size();
        cache.write(reinterpret_cast<const char*>(&req_size), sizeof(req_size));
        cache.write(reinterpret_cast<const char*>(pattern.required.data()), req_size * sizeof(size_t));
        
        size_t dis_size = pattern.disallowed.size();
        cache.write(reinterpret_cast<const char*>(&dis_size), sizeof(dis_size));
        cache.write(reinterpret_cast<const char*>(pattern.disallowed.data()), dis_size * sizeof(size_t));
    }
    
    return true;
}

bool BitFilterTree::LoadFiltersFromCache() {
    std::ifstream cache(kCachePath, std::ios::binary);
    if (!cache.is_open()) {
        return false;
    }
    
    size_t num_patterns;
    cache.read(reinterpret_cast<char*>(&num_patterns), sizeof(num_patterns));
    
    patterns_.clear();
    patterns_.reserve(num_patterns);
    
    for (size_t i = 0; i < num_patterns; i++) {
        FilterPattern pattern;
        size_t req_size;
        cache.read(reinterpret_cast<char*>(&req_size), sizeof(req_size));
        
        pattern.required.resize(req_size);
        cache.read(reinterpret_cast<char*>(pattern.required.data()), req_size * sizeof(size_t));
        
        // Read disallowed vector size and data
        size_t dis_size;
        cache.read(reinterpret_cast<char*>(&dis_size), sizeof(dis_size));
        pattern.disallowed.resize(dis_size);
        cache.read(reinterpret_cast<char*>(pattern.disallowed.data()), dis_size * sizeof(size_t));
        
        patterns_.push_back(pattern);
    }
    
    return true;
}

void BitFilterTree::GeneratePatternsRecursive(
    vector<FilterPattern>& patterns,
    const vector<size_t>& current_required,
    const vector<size_t>& current_disallowed,
    const vector<size_t>& available_positions,
    size_t start_idx
) {
    // For each available position, create a pattern where that position is required
    // and try each other available position as disallowed
    for (size_t i = start_idx; i < available_positions.size(); i++) {
        vector<size_t> new_required = current_required;
        new_required.push_back(available_positions[i]);
        
        // For each remaining position, create a pattern with it as disallowed
        for (size_t j = 0; j < available_positions.size(); j++) {
            if (j != i) {  // Skip the required position
                vector<size_t> new_disallowed = current_disallowed;
                new_disallowed.push_back(available_positions[j]);
                patterns.push_back({new_required, new_disallowed});
            }
        }
    }
}

// Add new helper method to filter patterns
vector<FilterPattern> BitFilterTree::FilterMatchingPatterns(const vector<FilterPattern>& input_patterns, 
                                                const FilterPattern& constraints) const {
    vector<FilterPattern> filtered;
    for (const auto& pattern : input_patterns) {
        bool satisfies_constraints = true;
        
        // Check required bits from constraints
        for (size_t req : constraints.required) {
            if (std::find(pattern.required.begin(), pattern.required.end(), req) == pattern.required.end()) {
                satisfies_constraints = false;
                break;
            }
        }
        
        // Check disallowed bits from constraints
        for (size_t dis : constraints.disallowed) {
            if (std::find(pattern.required.begin(), pattern.required.end(), dis) != pattern.required.end()) {
                satisfies_constraints = false;
                break;
            }
        }

        if (satisfies_constraints) {
            filtered.push_back(pattern);
        }
    }
    return filtered;
}

// Modify calculate_entropy to take filtered patterns
BitFilterTree::EntropyMetrics BitFilterTree::CalculateEntropy(
    size_t bit_position,
    const vector<FilterPattern>& filtered_patterns
) const {
    if (bit_position >= BITS) {
        return EntropyMetrics{0.0, 0.0, 0, 0, bit_position};
    }
    
    size_t matched = 0;
    size_t total = filtered_patterns.size();
    
    for (const auto& pattern : filtered_patterns) {
        if (find(pattern.required.begin(), pattern.required.end(), bit_position) 
            != pattern.required.end()) {
            matched++;
        }
    }

    double p = (total > 0) ? static_cast<double>(matched) / total : 0.0;
    
    EntropyMetrics result;
    result.entropy = (total > 0 && p > 0.0 && p < 1.0) 
        ? -p * log2(p) - (1-p) * log2(1-p)
        : 0.0;
    result.probability = p;
    result.match_count = matched;
    result.total_count = total;
    result.bit_index = bit_position;
    
    return result;
}

// Modify find_max_entropy_bit to use filtered patterns
BitFilterTree::EntropyMetrics BitFilterTree::FindMaxEntropyBit(
    const FilterPattern& constraints,
    const vector<FilterPattern>& filtered_patterns
) const {
    EntropyMetrics max_result{0.0, 0.0, 0, 0, 0};
    
    if (filtered_patterns.empty()) {
        return max_result;
    }
    
    vector<EntropyMetrics> thread_max_results(omp_get_max_threads(), max_result);
    
    #pragma omp parallel for
    for (size_t bit = 0; bit < BITS; bit++) {
        if (find(constraints.required.begin(), constraints.required.end(), bit) != constraints.required.end() ||
            find(constraints.disallowed.begin(), constraints.disallowed.end(), bit) != constraints.disallowed.end()) {
            continue;
        }

        EntropyMetrics current = CalculateEntropy(bit, filtered_patterns);

        int thread_id = omp_get_thread_num();
        if (current.entropy > thread_max_results[thread_id].entropy) {
            thread_max_results[thread_id] = current;
        }
    }

    for (const auto& thread_result : thread_max_results) {
        if (thread_result.entropy > max_result.entropy) {
            max_result = thread_result;
        }
    }

    return max_result;
}

// Modify build_tree to pass filtered patterns
unique_ptr<BitFilterTree::TreeNode> BitFilterTree::BuildTree(
    const FilterPattern& constraints,
    const vector<FilterPattern>& current_patterns,
    int depth,
    int max_depth,
    size_t min_patterns_leaf
) {
    auto node = make_unique<TreeNode>();
    
    cout<<depth<<" "<<current_patterns.size()<<endl;

    if (depth >= max_depth || current_patterns.empty() || 
        current_patterns.size() <= min_patterns_leaf || 
        std::all_of(current_patterns.begin() + 1, current_patterns.end(),
            [&](const FilterPattern& p) {
                return p.required == current_patterns[0].required &&
                        p.disallowed == current_patterns[0].disallowed;
            })) {
        node->is_leaf = true;
        node->patterns = current_patterns;
        return node;
    }

    node->metrics = FindMaxEntropyBit(constraints, current_patterns);

    // Create constraints and filter patterns for not matching case
    FilterPattern not_match_constraints = constraints;
    not_match_constraints.disallowed.push_back(node->metrics.bit_index);
    auto not_match_patterns = FilterMatchingPatterns(current_patterns, not_match_constraints);
    node->not_match = BuildTree(not_match_constraints, not_match_patterns, depth + 1, max_depth, min_patterns_leaf);

    // Create constraints and filter patterns for matching case
    FilterPattern match_constraints = constraints;
    match_constraints.required.push_back(node->metrics.bit_index);
    auto match_patterns = FilterMatchingPatterns(current_patterns, match_constraints);
    node->match = BuildTree(match_constraints, match_patterns, depth + 1, max_depth, min_patterns_leaf);

    return node;
}

BitFilterTree::TreeMetrics BitFilterTree::CalculateTreeStats(const TreeNode* node, size_t depth) const {
    if (!node) {
        return TreeMetrics{0.0, 0, 0};
    }

    if (node->is_leaf) {
        // For a leaf node:
        // - Each path to this leaf counts as one path
        // - Each path accumulates 'depth' comparisons from the decision nodes
        // - Each path needs ceil(patterns.size()/2) comparisons at the leaf
        size_t leaf_comparisons = (node->patterns.size() + 1) / 2;  // ceiling division
        size_t path_comparisons = depth + leaf_comparisons;
        return {
            static_cast<double>(path_comparisons),
            1,  // one path
            path_comparisons
        };
    }

    // Recursively calculate stats for both branches
    TreeMetrics left = CalculateTreeStats(node->not_match.get(), depth + 1);
    TreeMetrics right = CalculateTreeStats(node->match.get(), depth + 1);

    // Combine statistics
    return {
        (left.avg_comparisons * left.path_count + 
            right.avg_comparisons * right.path_count) / 
        (left.path_count + right.path_count),
        left.path_count + right.path_count,
        left.comparison_count + right.comparison_count
    };
}

// Add these new private methods
void BitFilterTree::SaveTreeBinaryRecursive(std::ofstream& out, const TreeNode* node) const {
    // Write if node exists
    bool exists = (node != nullptr);
    out.write(reinterpret_cast<const char*>(&exists), sizeof(exists));
    if (!exists) return;

    // Write node data
    out.write(reinterpret_cast<const char*>(&node->is_leaf), sizeof(node->is_leaf));
    out.write(reinterpret_cast<const char*>(&node->metrics), sizeof(node->metrics));

    if (node->is_leaf) {
        // Write patterns
        size_t patterns_size = node->patterns.size();
        out.write(reinterpret_cast<const char*>(&patterns_size), sizeof(patterns_size));
        
        for (const auto& pattern : node->patterns) {
            size_t req_size = pattern.required.size();
            size_t dis_size = pattern.disallowed.size();
            
            out.write(reinterpret_cast<const char*>(&req_size), sizeof(req_size));
            out.write(reinterpret_cast<const char*>(pattern.required.data()), req_size * sizeof(size_t));
            
            out.write(reinterpret_cast<const char*>(&dis_size), sizeof(dis_size));
            out.write(reinterpret_cast<const char*>(pattern.disallowed.data()), dis_size * sizeof(size_t));
        }
    }

    // Recursively save children
    SaveTreeBinaryRecursive(out, node->not_match.get());
    SaveTreeBinaryRecursive(out, node->match.get());
}

unique_ptr<BitFilterTree::TreeNode> BitFilterTree::LoadTreeBinaryRecursive(std::ifstream& in) const {
    // Read if node exists
    bool exists;
    in.read(reinterpret_cast<char*>(&exists), sizeof(exists));
    if (!exists) return nullptr;

    auto node = make_unique<TreeNode>();

    // Read node data
    in.read(reinterpret_cast<char*>(&node->is_leaf), sizeof(node->is_leaf));
    in.read(reinterpret_cast<char*>(&node->metrics), sizeof(node->metrics));

    if (node->is_leaf) {
        // Read patterns
        size_t patterns_size;
        in.read(reinterpret_cast<char*>(&patterns_size), sizeof(patterns_size));
        
        node->patterns.resize(patterns_size);
        for (size_t i = 0; i < patterns_size; i++) {
            FilterPattern pattern;
            size_t req_size, dis_size;
            
            in.read(reinterpret_cast<char*>(&req_size), sizeof(req_size));
            pattern.required.resize(req_size);
            in.read(reinterpret_cast<char*>(pattern.required.data()), req_size * sizeof(size_t));
            
            in.read(reinterpret_cast<char*>(&dis_size), sizeof(dis_size));
            pattern.disallowed.resize(dis_size);
            in.read(reinterpret_cast<char*>(pattern.disallowed.data()), dis_size * sizeof(size_t));
            
            node->patterns[i] = pattern;
        }
    }

    // Recursively load children
    node->not_match = LoadTreeBinaryRecursive(in);
    node->match = LoadTreeBinaryRecursive(in);

    return node;
}

bool BitFilterTree::ReadFiltersFromCsv(const string& filename) {
    if (LoadFiltersFromCache()) {
        cout << "Loaded filters from cache" << endl;
        
        return true;
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return false;
    }

    patterns_.clear();
    string line;
    while (std::getline(file, line)) {
        stringstream ss(line);
        string value;
        vector<size_t> row;
        
        try {
            while (std::getline(ss, value, ',')) {
                size_t pos = std::stoul(value);
                if (pos == 0) {
                    continue;
                }
                row.push_back(pos);
            }

            if (!row.empty()) {
                FilterPattern pattern;
                pattern.disallowed = {row.back()};  // Store last number in disallowed vector
                row.pop_back();  // Remove disallowed from positions
                pattern.required = row;  // Store remaining numbers as required
                patterns_.push_back(pattern);
            }
        
        } catch (const std::exception& e) {
            cerr << "Error parsing line: " << line << endl;
            cerr << "Exception: " << e.what() << endl;
        }
    }
    
    if (!patterns_.empty()) {
        if (SaveFiltersToCache()) {
            cout << "Saved filters to cache" << endl;
        } else {
            cerr << "Failed to save cache file" << endl;
        }
    }
    
    return !patterns_.empty();
}

const vector<FilterPattern>& BitFilterTree::GetPatterns() const {
    return patterns_;
}

vector<FilterPattern> BitFilterTree::GeneratePatterns(
    const vector<size_t>& initial_required,
    const vector<size_t>& initial_disallowed
) {
    vector<FilterPattern> patterns;
    
    // Get available positions (excluding initial required and disallowed bits)
    vector<size_t> available_positions;
    for (size_t i = 0; i < BITS; i++) {
        if (std::find(initial_required.begin(), initial_required.end(), i) == initial_required.end() &&
            std::find(initial_disallowed.begin(), initial_disallowed.end(), i) == initial_disallowed.end()) {
            available_positions.push_back(i);
        }
    }
    
    // Generate combinations using recursive helper
    GeneratePatternsRecursive(patterns, initial_required, initial_disallowed, available_positions, 0);
    return patterns;
}

BitFilterTree::EntropyMetrics BitFilterTree::CalculateEntropy(size_t bit_position, const FilterPattern& constraints) const {
    // Add early validation
    if (bit_position >= BITS) {
        return EntropyMetrics{0.0, 0.0, 0, 0, bit_position};
    }
    
    size_t matched = 0;
    size_t total = 0;
    
    // Check each pattern in our stored patterns
    for (const auto& stored_pattern : patterns_) {
        // Check if this pattern satisfies our constraints
        bool satisfies_constraints = true;
        
        // Check required bits from constraints
        for (size_t req : constraints.required) {
            if (std::find(stored_pattern.required.begin(), stored_pattern.required.end(), req) == stored_pattern.required.end()) {
                satisfies_constraints = false;
                break;
            }
        }
        
        // Check disallowed bits from constraints
        for (size_t dis : constraints.disallowed) {
            if (std::find(stored_pattern.required.begin(), stored_pattern.required.end(), dis) != stored_pattern.required.end()) {
                satisfies_constraints = false;
                break;
            }
        }

        if (satisfies_constraints) {
            // Check if bit is set in this pattern
            bool bit_is_set = std::find(stored_pattern.required.begin(), stored_pattern.required.end(), bit_position) != stored_pattern.required.end();
            
            if (bit_is_set) {
                matched++;
            }
            total++;
        }
    }

    // Protect against division by zero more explicitly
    double p = (total > 0) ? static_cast<double>(matched) / total : 0.0;
    
    // Move result construction to end for clarity
    EntropyMetrics result;
    result.entropy = (total > 0 && p > 0.0 && p < 1.0) 
        ? -p * std::log2(p) - (1-p) * std::log2(1-p)
        : 0.0;
    result.probability = p;
    result.match_count = matched;
    result.total_count = total;
    result.bit_index = bit_position;
    
    return result;
}

BitFilterTree::EntropyMetrics BitFilterTree::FindMaxEntropyBit(const FilterPattern& constraints) const {
    EntropyMetrics max_result{0.0, 0.0, 0, 0, 0};
    
    if (patterns_.empty()) {
        return max_result;
    }
    
    // Track max results per thread to avoid race condition
    vector<EntropyMetrics> thread_max_results(omp_get_max_threads(), max_result);
    
    #pragma omp parallel for
    for (size_t bit = 0; bit < BITS; bit++) {
        // Skip bits that are already in constraints
        if (std::find(constraints.required.begin(), constraints.required.end(), bit) != constraints.required.end() ||
            std::find(constraints.disallowed.begin(), constraints.disallowed.end(), bit) != constraints.disallowed.end()) {
            continue;
        }

        EntropyMetrics current = CalculateEntropy(bit, patterns_);

        // Update max result for this thread
        int thread_id = omp_get_thread_num();
        if (current.entropy > thread_max_results[thread_id].entropy) {
            thread_max_results[thread_id] = current;
        }
    }

    // Find global maximum from thread results
    for (const auto& thread_result : thread_max_results) {
        if (thread_result.entropy > max_result.entropy) {
            max_result = thread_result;
        }
    }

    return max_result;
}

// Modify BuildTree to store the result in root_
bool BitFilterTree::BuildTree(const FilterPattern& constraints, int max_depth, size_t min_patterns_leaf) {
    root_ = BuildTree(constraints, patterns_, 0, max_depth, min_patterns_leaf);
    return root_ != nullptr;
}

// Update the public methods to use root_
void BitFilterTree::PrintTree() const {
    PrintTreeHelper(root_.get(), 0);
}

// Rename the existing PrintTree to PrintTreeHelper and make it private
void BitFilterTree::PrintTreeHelper(const TreeNode* node, int depth) const {
    if (!node) return;

    string indent(depth * 2, ' ');
    cout << indent << "Bit " << node->metrics.bit_index 
            << " (entropy=" << fixed << setprecision(3) << node->metrics.entropy 
            << ", prob=" << node->metrics.probability
            << ", match_count=" << node->metrics.match_count
            << "/" << node->metrics.total_count << ")" << endl;
    
    if (node->not_match) {
        cout << indent << "  0 -> ";
        PrintTreeHelper(node->not_match.get(), depth + 1);
    }
    if (node->match) {
        cout << indent << "  1 -> ";
        PrintTreeHelper(node->match.get(), depth + 1);
    }
}

string BitFilterTree::CreateDotTree() const {
    return CreateDotTreeHelper(root_.get());
}

// Rename the existing CreateDotTree to CreateDotTreeHelper and make it private
string BitFilterTree::CreateDotTreeHelper(const TreeNode* node) const {
    stringstream ss;
    ss << "strict digraph decision_tree {\n";
    ss << "  node [shape=box];\n";
    
    // Calculate tree statistics first
    TreeMetrics stats = AnalyzeTree();
    
    // Add overall statistics to the graph
    ss << "  graph [label=\"Average comparisons: " 
        << fixed << setprecision(2) << stats.avg_comparisons 
        << "\\nTotal paths: " << stats.path_count 
        << "\\nTotal comparisons: " << stats.comparison_count 
        << "\"];\n";
    
    // Helper function for recursive node printing
    std::function<void(const TreeNode*, size_t)> print_node = [&](const TreeNode* n, size_t id) {
        if (!n) return;
        
        if (n->is_leaf) {
            size_t leaf_comparisons = (n->patterns.size() + 1) / 2;
            ss << "  \"node" << id << "\" [label=\"Leaf\\n" 
                << n->patterns.size() << " patterns\\n"
                << "comparisons: " << leaf_comparisons << "\"];\n";
        } else {
            ss << "  \"node" << id << "\" [label=\"Bit " << n->metrics.bit_index 
                << "\\nentropy=" << fixed << setprecision(3) << n->metrics.entropy 
                << "\\nprob=" << n->metrics.probability 
                << "\\n" << n->metrics.match_count << "/" << n->metrics.total_count << "\"];\n";
            
            if (n->not_match) {
                ss << "  \"node" << id << "\" -> \"node" << (id * 2) << "\" [label=\"0\"];\n";
                print_node(n->not_match.get(), id * 2);
            }
            
            if (n->match) {
                ss << "  \"node" << id << "\" -> \"node" << (id * 2 + 1) << "\" [label=\"1\"];\n";
                print_node(n->match.get(), id * 2 + 1);
            }
        }
    };
    
    print_node(node, 1);
    ss << "}\n";
    return ss.str();
}

bool BitFilterTree::ClassifyPattern(const vector<size_t>& set_bits, size_t max_bits) const {
    return ClassifyPatternHelper(set_bits, root_.get(), max_bits);
}

bool BitFilterTree::ClassifyPatternHelper(const vector<size_t>& set_bits, const TreeNode* node, size_t max_bits) const {
    if (!node) {
        return false;
    }

    // If we've reached a leaf node, check if any pattern matches our constraints
    if (node->is_leaf) {
        for (const auto& leaf_pattern : node->patterns) {
            
            bool required_pass = true;
            for (const auto& req : leaf_pattern.required) {
                if (std::find(set_bits.begin(), set_bits.end(), req) == set_bits.end()) { 
                    // if the required for this leaf pattern is not in the query pattern
                    required_pass = false;
                    // cout<<"required not found "<<req<<endl;
                    break;
                }
            }

            bool disallowed_fail = false;
            if (required_pass) {
                
                for (const auto& dis : leaf_pattern.disallowed) {
                    if (dis > max_bits || std::find(set_bits.begin(), set_bits.end(), dis) != set_bits.end()) {
                        disallowed_fail = true;
                        break;
                    }
                }
            }
            // cout<<"disallowed_fail "<<disallowed_fail<<" required_pass "<<required_pass<<endl;
            // cout<<"return "<<(required_pass == true && disallowed_fail == false)<<endl;
            // cout<<"------"<<endl;

            if (required_pass == true && disallowed_fail == false) {
                return true;
            }
        }
        return false;
    }

    // For non-leaf nodes, check the current bit position
    size_t current_bit = node->metrics.bit_index;
  
    // If the current bit is not in set_bits, follow the not_match path
    if (std::find(set_bits.begin(), set_bits.end(), current_bit) == set_bits.end()) {
        return ClassifyPatternHelper(set_bits, node->not_match.get(), max_bits);
    }
    
    return ClassifyPatternHelper(set_bits, node->match.get(), max_bits) || 
           ClassifyPatternHelper(set_bits, node->not_match.get(), max_bits);
}

BitFilterTree::TreeMetrics BitFilterTree::AnalyzeTree() const {
    return CalculateTreeStats(root_.get());
}

bool BitFilterTree::SaveTreeBinary(const string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }
    
    SaveTreeBinaryRecursive(out, root_.get());
    return true;
}

bool BitFilterTree::LoadTreeBinary(const string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    
    root_ = LoadTreeBinaryRecursive(in);
    return root_ != nullptr;
}



// int main() {
//     BitFilterTree filter_tree;
    
//     cout << "Reading filters" << endl;
//     if (!filter_tree.ReadFiltersFromCsv("diophantine_small.txt")) {
//         cerr << "Failed to read filters" << endl;
//         return 1;
//     }

//     cout << "Building Tree" <<endl;
//     auto root = filter_tree.BuildTree(FilterPattern(), 0, 40, 1);
    
//     // Get and print tree statistics
//     cout << "Analyzing Tree" <<endl;
//     auto stats = filter_tree.AnalyzeTree(root.get());
//     cout << "Tree Statistics:" << endl;
//     cout << "Average comparisons per path: " << stats.avg_comparisons << endl;
//     cout << "Total paths: " << stats.path_count << endl;
//     cout << "Total comparisons: " << stats.comparison_count << endl;
    
//     // Create DOT file with statistics
//     // auto dot_tree = filter_tree.CreateDotTree(root.get());
//     // std::ofstream dot_file("tree.dot");
//     // if (dot_file.is_open()) {
//     //     dot_file << dot_tree;
//     //     dot_file.close();
//     // }

//     cout << "Saving tree in binary format..." << endl;
//     if (!filter_tree.SaveTreeBinary(root.get(), "tree.bin")) {
//         cerr << "Failed to save tree in binary format" << endl;
//         return 1;
//     }
//     cout << "Tree saved successfully to tree.bin" << endl;

//     // To load the tree later:
//     cout << "Loading tree from binary file..." << endl;
//     auto loaded_root = filter_tree.LoadTreeBinary("tree.bin");
//     if (!loaded_root) {
//         cerr << "Failed to load tree from binary file" << endl;
//         return 1;
//     }
//     cout << "Tree loaded successfully" << endl;

//     FilterPattern pattern;
//     pattern.required = {3, 4};    // bits that must be 1
//     pattern.disallowed = {12};       // bits that must be 0
//     cout<<filter_tree.ClassifyPattern(pattern, root.get())<<endl;

//     FilterPattern pattern1;
//     pattern1.required = {6,8,10};    // bits that must be 1
//     pattern1.disallowed = {12};       // bits that must be 0
//     cout<<filter_tree.ClassifyPattern(pattern1, root.get())<<endl;
//     return 0;
// }