#ifndef BITFILTERTREE_H
#define BITFILTERTREE_H

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <bitset>


struct FilterPattern {
    __uint128_t required = 0;  // Bits that must be 1
    __uint128_t disallowed = 0; // Bits that must be 0
};

class BitFilterTree {
public:
    explicit BitFilterTree(size_t max_bits);


    struct EntropyMetrics {
        double entropy;
        double probability;
        size_t match_count;
        size_t total_count;
        size_t bit_index;
    };

    struct TreeNode {
        bool is_leaf = false;
        EntropyMetrics metrics;
        std::unique_ptr<TreeNode> not_match;   // when bit is 0
        std::unique_ptr<TreeNode> match;       // when bit is 1
        std::vector<FilterPattern> patterns;
    };

    struct TreeMetrics {
        double avg_comparisons;
        size_t path_count;
        size_t comparison_count;
    };

    // Public methods
    bool ReadFiltersFromCsv(const std::string& filename, const std::string& cache_path);
    void BuildTreeFile(const std::string& tree_path, int max_depth, int min_patterns_leaf);
    const std::vector<FilterPattern>& GetPatterns() const;
    std::vector<FilterPattern> GeneratePatterns(const std::vector<size_t>& initial_required,
                                               const std::vector<size_t>& initial_disallowed);
    EntropyMetrics CalculateEntropy(size_t bit_position, const FilterPattern& constraints) const;
    EntropyMetrics FindMaxEntropyBit(const FilterPattern& constraints) const;
    bool BuildTree(const FilterPattern& constraints = FilterPattern(),
                  int max_depth = 40,
                  size_t min_patterns_leaf = 1);
    void PrintTree() const;
    std::string CreateDotTree() const;
    bool ClassifyPattern(__uint128_t set_bits) const;
    TreeMetrics AnalyzeTree() const;
    bool SaveTreeBinary(const std::string& filename) const;
    bool LoadTreeBinary(const std::string& filename);
    bool SaveFiltersToCache(const std::string& cache_path);
    bool LoadFiltersFromCache(const std::string& cache_path);

private:
    const size_t BITS;
    std::vector<FilterPattern> patterns_;
    std::unique_ptr<TreeNode> root_;

    // high_bit_mask is a way to ignore patterns that involve bits higher than
    // exist in the pile we are considering. If disallowed & high_bit_mask!=0 then we 
    // ignore this pattern
    __uint128_t high_bit_mask;  
    
    static inline void PrintBits(__uint128_t value) {
        // Split the 128-bit value into two 64-bit parts
        uint64_t lower = static_cast<uint64_t>(value);         // Lower 64 bits
        uint64_t upper = static_cast<uint64_t>(value >> 64);   // Upper 64 bits

        // Print the upper and lower parts in binary
        std::cout << std::bitset<64>(upper) << std::bitset<64>(lower) << std::endl;
    }

    // Helper functions for bit operations
    static inline bool IsBitSet(__uint128_t value, size_t bit) {
        return (value & ((__uint128_t)1 << bit)) != 0;
    }

    static inline void SetBit(__uint128_t& value, size_t bit) {
        if (bit < 128) {
            value |= (__uint128_t(1) << bit);
        }
    }
    
    // Private helper methods
    void GeneratePatternsRecursive(std::vector<FilterPattern>& patterns,
                                  const std::vector<size_t>& current_required,
                                  const std::vector<size_t>& current_disallowed,
                                  const std::vector<size_t>& available_positions,
                                  size_t start_idx);
    std::vector<FilterPattern> FilterMatchingPatterns(const std::vector<FilterPattern>& input_patterns,
                                                     const FilterPattern& constraints) const;
    EntropyMetrics CalculateEntropy(size_t bit_position,
                                   const std::vector<FilterPattern>& filtered_patterns) const;
    EntropyMetrics FindMaxEntropyBit(const FilterPattern& constraints,
                                    const std::vector<FilterPattern>& filtered_patterns) const;
    std::unique_ptr<TreeNode> BuildTree(const FilterPattern& constraints,
                                       const std::vector<FilterPattern>& current_patterns,
                                       int depth,
                                       int max_depth,
                                       size_t min_patterns_leaf);
    void PrintTreeHelper(const TreeNode* node, int depth) const;
    std::string CreateDotTreeHelper(const TreeNode* node) const;
    bool ClassifyPatternHelper(__uint128_t set_bits, const TreeNode* node) const;
    TreeMetrics CalculateTreeStats(const TreeNode* node, size_t depth = 0) const;
    void SaveTreeBinaryRecursive(std::ofstream& out, const TreeNode* node) const;
    std::unique_ptr<TreeNode> LoadTreeBinaryRecursive(std::ifstream& in) const;
};

#endif // BITFILTERTREE_H
