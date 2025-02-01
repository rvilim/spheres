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

BitFilterTree::BitFilterTree(size_t max_bits) : BITS(max_bits) {
    // Initialize root_ as nullptr
    root_ = nullptr;
    // Create mask with 1s for all bits above max_bits
    high_bit_mask = 0;
    for (size_t i = max_bits; i < 128; i++) {
        SetBit(high_bit_mask, i);
    }
}

bool BitFilterTree::SaveFiltersToCache() {
    std::ofstream cache(kCachePath, std::ios::binary);
    if (!cache.is_open()) {
        return false;
    }
    
    size_t num_patterns = patterns_.size();
    cache.write(reinterpret_cast<const char*>(&num_patterns), sizeof(num_patterns));
    
    for (const auto& pattern : patterns_) {
        cache.write(reinterpret_cast<const char*>(&pattern.required), sizeof(__uint128_t));
        cache.write(reinterpret_cast<const char*>(&pattern.disallowed), sizeof(__uint128_t));
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
        cache.read(reinterpret_cast<char*>(&pattern.required), sizeof(__uint128_t));
        cache.read(reinterpret_cast<char*>(&pattern.disallowed), sizeof(__uint128_t));
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
        // Create new required pattern with current bit set
        FilterPattern pattern{0, 0};  // Initialize both fields to 0
        
        // Set all current required bits
        for (size_t req_bit : current_required) {
            SetBit(pattern.required, req_bit);
        }
        // Add the new required bit
        SetBit(pattern.required, available_positions[i]);
        
        // Set all current disallowed bits
        for (size_t dis_bit : current_disallowed) {
            SetBit(pattern.disallowed, dis_bit);
        }
        
        // For each remaining position, create a pattern with it as disallowed
        for (size_t j = 0; j < available_positions.size(); j++) {
            if (j != i) {  // Skip the required position
                // Create a copy of the current pattern and add the new disallowed bit
                FilterPattern new_pattern = pattern;
                SetBit(new_pattern.disallowed, available_positions[j]);
                patterns.push_back(new_pattern);
            }
        }
    }
}

// Add new helper method to filter patterns
vector<FilterPattern> BitFilterTree::FilterMatchingPatterns(
    const vector<FilterPattern>& input_patterns,
    const FilterPattern& constraints) const 
{
    vector<FilterPattern> filtered;
    for (const auto& pattern : input_patterns) {
        // Check if pattern satisfies constraints:
        // 1. All required bits in constraints must be set in pattern.required
        // 2. No disallowed bits in constraints can be set in pattern.required
        if ((pattern.required & constraints.required) == constraints.required &&
            (pattern.required & constraints.disallowed) == 0) {
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
        if (IsBitSet(pattern.required, bit_position)) {
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
        if (IsBitSet(constraints.required, bit) ||
            IsBitSet(constraints.disallowed, bit)) {
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
    static size_t total_leaf_patterns = 0;
    static const size_t PROGRESS_INTERVAL = 10000;
    
    auto node = make_unique<TreeNode>();
    
    if (depth >= max_depth || current_patterns.empty() || 
        current_patterns.size() <= min_patterns_leaf || 
        std::all_of(current_patterns.begin() + 1, current_patterns.end(),
            [&](const FilterPattern& p) {
                return p.required == current_patterns[0].required &&
                        p.disallowed == current_patterns[0].disallowed;
            })) {
        node->is_leaf = true;
        node->patterns = current_patterns;

        // Update total and print progress
        total_leaf_patterns += current_patterns.size();
        if (total_leaf_patterns / PROGRESS_INTERVAL > (total_leaf_patterns - current_patterns.size()) / PROGRESS_INTERVAL) {
            cout << "Processed " << total_leaf_patterns << "/" << patterns_.size() 
                 << " patterns in leaf nodes..." << endl;
        }
      
        return node;
    }

    node->metrics = FindMaxEntropyBit(constraints, current_patterns);

    // Create constraints and filter patterns for not matching case
    FilterPattern not_match_constraints = constraints;
    SetBit(not_match_constraints.disallowed, node->metrics.bit_index);
    auto not_match_patterns = FilterMatchingPatterns(current_patterns, not_match_constraints);
    node->not_match = BuildTree(not_match_constraints, not_match_patterns, depth + 1, max_depth, min_patterns_leaf);

    // Create constraints and filter patterns for matching case
    FilterPattern match_constraints = constraints;
    SetBit(match_constraints.required, node->metrics.bit_index);
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
            out.write(reinterpret_cast<const char*>(&pattern.required), sizeof(__uint128_t));
            out.write(reinterpret_cast<const char*>(&pattern.disallowed), sizeof(__uint128_t));
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
            in.read(reinterpret_cast<char*>(&pattern.required), sizeof(__uint128_t));
            in.read(reinterpret_cast<char*>(&pattern.disallowed), sizeof(__uint128_t));
            node->patterns[i] = pattern;
        }
    }

    // Recursively load children
    node->not_match = LoadTreeBinaryRecursive(in);
    node->match = LoadTreeBinaryRecursive(in);

    return node;
}

bool BitFilterTree::ReadFiltersFromCsv(const string& filename) {
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
            // Parse all numbers from the line
            while (std::getline(ss, value, ',')) {
                // Convert string to size_t, handling whitespace
                value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());
                if (value.empty()) continue;
                
                size_t pos = std::stoul(value);
                if (pos == 0) continue;
                
                // Adjust position to be 0-based
                pos--;
                if (pos >= BITS) {
                    cerr << "Warning: Position " << (pos + 1) << " exceeds bit limit" << endl;
                    continue;
                }
                row.push_back(pos);
            }
            if (!row.empty()) {
                FilterPattern pattern{0, 0};  // Initialize both fields to 0
                for (int i=0;i<row.size()-1;i++){
                    SetBit(pattern.required, row[i]);
                }

                SetBit(pattern.disallowed, row[row.size()-1]);
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
        if ((stored_pattern.required & constraints.required) != constraints.required) {
            satisfies_constraints = false;
        }
        
        // Check disallowed bits from constraints
        if ((stored_pattern.required & constraints.disallowed) != 0) {
            satisfies_constraints = false;
        }

        if (satisfies_constraints) {
            // Check if bit is set in this pattern
            bool bit_is_set = IsBitSet(stored_pattern.required, bit_position);
            
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
        if (IsBitSet(constraints.required, bit) ||
            IsBitSet(constraints.disallowed, bit)) {
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

bool BitFilterTree::ClassifyPattern(__uint128_t set_bits) const {
    return ClassifyPatternHelper(set_bits, root_.get());
}

bool BitFilterTree::ClassifyPatternHelper(__uint128_t set_bits, const TreeNode* node) const {
    if (!node) {
        return false;
    }

    // If we've reached a leaf node, check if any pattern matches our constraints
    if (node->is_leaf) {
        for (const auto& leaf_pattern : node->patterns) {
            auto required_set =  (leaf_pattern.required & set_bits) == leaf_pattern.required;
            auto disallowed_unset = (leaf_pattern.disallowed & set_bits) == 0;

            // If this pattern requires that we don't set a bit higher than max_bits, then we did not match the pattern
            auto max_exceeded = (high_bit_mask & leaf_pattern.disallowed) != 0;

            if ( required_set && disallowed_unset && (!max_exceeded)){
                cout<<"---"<<endl;
                PrintBits(leaf_pattern.required);
                PrintBits(leaf_pattern.disallowed);
                PrintBits(set_bits);
                cout<<"---"<<endl;
                return true;
            }
        }
        return false;
    }

    // For non-leaf nodes, check the current bit position
    size_t current_bit = node->metrics.bit_index;
  
    // If the current bit is not set in set_bits, follow the not_match path
    if (!IsBitSet(set_bits, current_bit)) {
        return ClassifyPatternHelper(set_bits, node->not_match.get());
    }
    
    return ClassifyPatternHelper(set_bits, node->match.get()) || 
           ClassifyPatternHelper(set_bits, node->not_match.get());
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