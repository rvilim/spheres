#include <iostream>
#include <utility>
#include <vector>
#include <fstream>
#include "diophantine.h"
#include <boost/dynamic_bitset.hpp>
#include "defs.h"

using namespace std;
using namespace boost;

void Filter::add(dynamic_bitset<> lhs, int rhs){
    auto cur_node = &filter_root.targets[rhs];
    int max_set=0;

    for(int i=lhs.size()-1; i>=0;i--){
        if (lhs[i]){
            max_set = i;
            break;
        }
    }

    for(int i=0;i<=max_set;i++){
        if (lhs[i]==0){
            if (cur_node->left == nullptr){
                auto new_node = new filter_node();
                cur_node->left = new_node;
            }
            cur_node = cur_node->left;
        }else{
            if (cur_node->right == nullptr){
                auto new_node = new filter_node();
                cur_node->right = new_node;
            }
            cur_node = cur_node->right;
        }
    }
    cur_node->end=true;
}

bool Filter::m(vector<bool> &pile_mask, filter_node &cur_node, int cur_pos){
    bool res=false;
//    vector<int_fast8_t> pile = {0,0,1,1,0,0,1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,7,6,5,4,3,2,1};
//    3 7 11 12 18 filtering 21

    if ((cur_node.end) && (pile_mask[cur_pos])) {
        // If the left node and the right node are both null we have
        // matched a mask and should filter
        return true;
    }

    if ((cur_node.left == nullptr) && (cur_node.right != nullptr)) {
        // If the left node is a null pointer, and the right node is _not_ null
        // it means that we have to have a 1 in the pile in order to continue
        if  (pile_mask[cur_pos]) {
            res |= m(pile_mask, *cur_node.right, cur_pos + 1);
        }else {
            return false;
        }
    }else if ((cur_node.left != nullptr) && (cur_node.right == nullptr)) {
        res |= m(pile_mask, *cur_node.left, cur_pos + 1);
    }else if ((cur_node.left != nullptr) && (cur_node.right != nullptr)) {
        // If there are filters for both set and unset bits in the piles
        // we go either left or right depending on our current pile mask
        if  (pile_mask[cur_pos]) {
            res |= m(pile_mask, *cur_node.right, cur_pos + 1);
        }else {
            res |= m(pile_mask, *cur_node.left, cur_pos + 1);
        }
    }

    return res;
}

bool Filter::filter_single(vector<bool> pile_mask, int target){
    return m(pile_mask, filter_root.targets[target], 0);
}
bool Filter::filter(vector<bool> pile_mask){
    // This will return true if we _should_ filter this pile (e.g. remove it)
    // This occurs if there is any filter where: filter & mask = filter

    for(int i=5;i<pile_mask.size();i++){
        auto cube_target = i+1;
        if (!pile_mask[i] && (filter_single(pile_mask, cube_target))){
            return true;
        }
    }
    return false;
}

void Filter::read_filters(){
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        int8_t rhs;

        {
            auto pair = split(string(line), '|');
            auto lhs_s = split(pair[0], ',');
            int_fast8_t max_set=0;

            rhs = stoi(pair[1]);

            if (rhs>n_cubes) continue;

            for(auto & i : lhs_s) {
                if(stoi(i)>max_set) max_set = stoi(i);
            }

            dynamic_bitset<> lhs_bits(max_set);
            for(auto &i : lhs_s)  lhs_bits[stoi(i)-1]=true;
            add(lhs_bits, rhs);
        }

    }
    infile.close();
}


std::vector<std::string> Filter::split(const std::string& str, char delim) {
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}
/*
vector<vector<vector<int_fast8_t>>> read_filters(string filename, int n_cubes){
    filter_node filters(100);
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        vector<int_fast8_t> lhs;
        int8_t rhs;

        {
            auto pair = split(string(line), '|');
            auto lhs_s = split(pair[0], ',');

            rhs = stoi(pair[1]);

            if (rhs>n_cubes) continue;

            for(auto & i : lhs_s){
                if (stoi(i)>n_cubes) continue;
                lhs.push_back(stoi(i));
            }
        }
        filters[rhs].push_back(lhs);
    }
    infile.close();

    return filters;
}*/


std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        strings.push_back(str.substr(start, end - start));
    }
    return strings;
}

vector<vector<vector<int_fast8_t>>> read_filters(string filename, int n_cubes){
    vector<vector<vector<int_fast8_t>>> filters(100);
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        vector<int_fast8_t> lhs;
        int8_t rhs;

        {
            auto pair = split(string(line), '|');
            auto lhs_s = split(pair[0], ',');

            rhs = stoi(pair[1]);

            if (rhs>n_cubes) continue;

            for(auto & i : lhs_s){
                if (stoi(i)>n_cubes) continue;
                lhs.push_back(stoi(i));
            }
        }
        filters[rhs].push_back(lhs);
    }
    infile.close();

    return filters;
}

bool filter_diophantine_old(vector<int_fast8_t> &pile, int pile_num){
    // We start this loop at 5 because there are no entries < 6 in the filters rhs table
    for(int i=5;i<pile.size();i++){
        if (pile[i]==0){
            auto current_cube=i+1;
            for (auto & filter : filters[current_cube]){
                bool do_filter = true;
                for (auto & filter_cube : filter){;
                    if (pile[filter_cube-1]!=pile_num){
                        do_filter=false;
                        break;
                    }
                }

                if (do_filter){
                        for (auto & filter_cube : filter) {
                            cout << unsigned(filter_cube )<< " ";
                        }
                        cout<<"filtering "<<current_cube<<endl;
                    return true;
                }

            }
        }
    }

    return false;
}