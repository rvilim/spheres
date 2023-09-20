//
// Created by Ryan Vilim on 3/29/22.
//

#ifndef PILES_DIOPHANTINE_H
#define PILES_DIOPHANTINE_H

#include <utility>
#include <vector>
#include <map>
#include <boost/dynamic_bitset.hpp>
#include <unordered_map>

using namespace std;
using namespace boost;

struct FilterNode {
    struct FilterNode *children[100];
    bool end;
};

class Filter {
public:
    Filter(string filename, int n_cubes) : filename(std::move(filename)), n_cubes(n_cubes) {
        read_filters();
    }
//    bool search(int rhs,  vector<uint8_t> key);
    bool find(const vector<int>& pile);
private:
    string filename;
    int n_cubes;

    unordered_map<int, vector<vector<int>>> filters;
    static std::vector<std::string> split(const std::string& str, char delim);


    void read_filters();

    };

    bool filter_diophantine_old(vector<int_fast8_t> &pile, int pile_num);

#endif //PILES_DIOPHANTINE_H

