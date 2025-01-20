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

const size_t BITS = 70;

struct FilterPattern {
    vector<size_t> required;
    vector<size_t> disallowed;
};



class BitFilterTree1 {
private:
    string cache_path = "diophantine1.cache";
    vector<FilterPattern> patterns;
    
    bool save_filters_to_cache() {
        std::ofstream cache(cache_path, std::ios::binary);
        if (!cache.is_open()) {
            return false;
        }
        
        size_t num_patterns = patterns.size();
        cache.write(reinterpret_cast<const char*>(&num_patterns), sizeof(num_patterns));
        
        for (const auto& pattern : patterns) {
            size_t req_size = pattern.required.size();
            cache.write(reinterpret_cast<const char*>(&req_size), sizeof(req_size));
            cache.write(reinterpret_cast<const char*>(pattern.required.data()), req_size * sizeof(size_t));
            
            // Write disallowed vector size and data
            size_t dis_size = pattern.disallowed.size();
            cache.write(reinterpret_cast<const char*>(&dis_size), sizeof(dis_size));
            cache.write(reinterpret_cast<const char*>(pattern.disallowed.data()), dis_size * sizeof(size_t));
        }
        
        return true;
    }
    
    bool load_filters_from_cache() {
        std::ifstream cache(cache_path, std::ios::binary);
        if (!cache.is_open()) {
            return false;
        }
        
        size_t num_patterns;
        cache.read(reinterpret_cast<char*>(&num_patterns), sizeof(num_patterns));
        
        patterns.clear();
        patterns.reserve(num_patterns);
        
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
            
            patterns.push_back(pattern);
        }
        
        return true;
    }

    void generate_patterns_recursive(
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

    struct EntropyResult {
        double entropy;
        double probability;
        size_t matched;
        size_t total;
        size_t bit_position;
    };



public:
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

        patterns.clear();
        string line;

        while (std::getline(file, line)) {
            stringstream ss(line);
            string value;
            vector<size_t> row;
            
            try {
                while (std::getline(ss, value, ',')) {
                    size_t pos = std::stoul(value);
                    if (pos == 0 || pos > BITS) {
                        continue;
                    }
                    row.push_back(pos);
                }
                
                if (!row.empty()) {
                    FilterPattern pattern;
                    pattern.disallowed = {row.back()};  // Store last number in disallowed vector
                    row.pop_back();  // Remove disallowed from positions
                    pattern.required = std::move(row);  // Store remaining numbers as required
                    patterns.push_back(pattern);
                }
            } catch (const std::exception& e) {
                cerr << "Error parsing line: " << line << endl;
                cerr << "Exception: " << e.what() << endl;
            }
        }
        
        if (!patterns.empty()) {
            if (save_filters_to_cache()) {
                cout << "Saved filters to cache" << endl;
            } else {
                cerr << "Failed to save cache file" << endl;
            }
        }
        
        return !patterns.empty();
    }

    const vector<FilterPattern>& get_patterns() const {
        return patterns;
    }

    vector<FilterPattern> generate_patterns(
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
        generate_patterns_recursive(patterns, initial_required, initial_disallowed, available_positions, 0);
        return patterns;
    }

    EntropyResult calculate_entropy(size_t bit_position, const FilterPattern& constraints) const {
        // constraints are used to filter the filters but do not count as matching/not matching for entropy calculations
        
        size_t matched = 0;
        size_t total = 0;
        EntropyResult result;

        // Check each pattern in our stored patterns
        for (const auto& stored_pattern : patterns) {
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

        double p = static_cast<double>(matched) / total;
        
        if (total == 0 || p == 0.0 || p == 1.0) {
            result.entropy = 0.0;
        } else {
            result.entropy = -p * std::log2(p) - (1-p) * std::log2(1-p);
        }
        
        result.probability = p;
        result.matched = matched;
        result.total = total;
        result.bit_position = bit_position;
        
        return result;
    }

    EntropyResult find_max_entropy_bit(const FilterPattern& constraints) const {
        EntropyResult max_result;
        max_result.entropy = -1.0;  // Initialize to negative value to ensure we find a maximum

        #pragma omp parallel for
        for (size_t bit = 0; bit < BITS; bit++) {
            // Skip bits that are already in constraints
            if (std::find(constraints.required.begin(), constraints.required.end(), bit) != constraints.required.end() ||
                std::find(constraints.disallowed.begin(), constraints.disallowed.end(), bit) != constraints.disallowed.end()) {
                continue;
            }

            EntropyResult current = calculate_entropy(bit, constraints);
            
            #pragma omp critical
            {
                if (current.entropy > max_result.entropy) {
                    max_result = current;
                }
            }
        }

        return max_result;
    }
};





int main() {
    BitFilterTree1 filter_tree;
    
    cout << "Reading filters" << endl;
    if (!filter_tree.read_filters_from_csv("../diophantine.txt")) {
        cerr << "Failed to read filters" << endl;
        return 1;
    }

    // Find the bit with maximum entropy
    cout << "Finding bit with maximum entropy..." << endl;
    FilterPattern constraints;  // Empty constraints
    auto max_entropy_result = filter_tree.find_max_entropy_bit(constraints);
    
    cout << "Bit with maximum entropy: " << max_entropy_result.bit_position << endl;
    cout << "Entropy: " << max_entropy_result.entropy << endl;
    cout << "Probability: " << max_entropy_result.probability << endl;
    cout << "Matched/Total: " << max_entropy_result.matched << "/" << max_entropy_result.total << endl;



    return 0;
}