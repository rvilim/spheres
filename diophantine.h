//
// Created by Ryan Vilim on 3/29/22.
//

#ifndef PILES_DIOPHANTINE_H
#define PILES_DIOPHANTINE_H

#include <utility>
#include <vector>
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
        for(int i=1;i<=n_cubes;i++){
            filter_roots.push_back({});
        }
        read_filters();
    }
    bool search(int rhs,  vector<uint8_t> key);

private:
    string filename;
    int n_cubes;

    vector<FilterNode> filter_roots;

    void add(struct FilterNode *root, vector<uint8_t> key);
    static std::vector<std::string> split(const std::string& str, char delim);
//    bool m(vector<bool> &pile_mask, filter_node &cur_node, int cur_pos);


    void read_filters();

    };

    bool filter_diophantine_old(vector<int_fast8_t> &pile, int pile_num);

#endif //PILES_DIOPHANTINE_H

