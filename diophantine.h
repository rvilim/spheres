//
// Created by Ryan Vilim on 3/29/22.
//

#ifndef PILES_DIOPHANTINE_H
#define PILES_DIOPHANTINE_H

#include <utility>
#include <vector>
#include <boost/dynamic_bitset.hpp>

using namespace std;
using namespace boost;

struct filter_node {
    bool end = false;
    filter_node* left = nullptr;
    filter_node* right = nullptr;
};

struct filter_tree {
    vector<filter_node> targets;
};

class Filter {
public:
    Filter(string filename, int n_cubes) : filename(std::move(filename)), n_cubes(n_cubes) {
        for(int i=1;i<=n_cubes;i++){
            filter_root.targets.push_back({false, nullptr, nullptr});
        }
        read_filters();
    }

    bool filter(vector<bool> pile_mask);
    bool filter_single(vector<bool> pile_mask, int target);

private:
    string filename;
    int n_cubes;

    filter_tree filter_root;

    void add(dynamic_bitset<> lhs, int rhs);
    static std::vector<std::string> split(const std::string& str, char delim);
    bool m(vector<bool> &pile_mask, filter_node &cur_node, int cur_pos);


    void read_filters();

    };

    bool filter_diophantine_old(vector<int_fast8_t> &pile, int pile_num);

#endif //PILES_DIOPHANTINE_H

