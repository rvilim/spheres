#include <iostream>
#include <utility>
#include <vector>
#include <fstream>
#include "diophantine.h"
#include <boost/dynamic_bitset.hpp>
#include "defs.h"

using namespace std;
using namespace boost;

struct FilterNode *getNode(void) {
    auto *pNode = new FilterNode;
    pNode->end=false;

    for(auto & i : pNode->children){
        i= nullptr;
    }
    return pNode;
}

void Filter::add(struct FilterNode *root, vector<uint8_t> key){
    struct FilterNode *pCrawl = root;

    for(auto & index: key){
        if (!pCrawl->children[index]) {
            pCrawl->children[index]=getNode();
        }

        pCrawl = pCrawl->children[index];
    }
    pCrawl->end = true;
}

bool s(vector<uint8_t> &key, int idx, FilterNode *node){

    if (node->end){
        return true;
    }
    if (idx==key.size()) {
        return false;
    }

    bool ret = s(key, idx + 1, node);

    if (node->children[key[idx]]) {
        ret |= s(key, idx + 1, node->children[key[idx]]);
    }

    return ret;
}

bool Filter::search(int rhs,  vector<uint8_t> key){
    return s(key, 0, &filter_roots[rhs]);
}

void Filter::read_filters(){
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        int8_t rhs;
        vector<uint8_t> lhs;

        {
            auto pair = split(string(line), '|');
            auto lhs_s = split(pair[0], ',');

            rhs = stoi(pair[1]);


            if (rhs>n_cubes) continue;

            for(auto & i : lhs_s) {
                lhs.push_back(stoi(i));
            }

            add(&filter_roots[rhs], lhs);
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
//
//int main(){
//    auto f = Filter("/Users/rvilim/repos/piles/cubes_8",100);
//
//    cout<<"Should be true "<<f.search(9,{1,6,7,8})<<endl;
//    cout<<"Should be false "<<f.search(9,{1,7,8})<<endl;
//    cout<<"Should be true "<<f.search(25,{1,4,15,17,18,22})<<endl;
//    cout<<"Should be true "<<f.search(25,{4,15,17,18,22})<<endl;
//    cout<<"Should be false "<<f.search(25,{15,17,18,22,38})<<endl;
//    cout<<"Should be true "<<f.search(25,{4,15,17,18,22,38})<<endl;
//
//
//}