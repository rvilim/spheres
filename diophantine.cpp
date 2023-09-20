#include <iostream>
#include <utility>
#include <vector>
#include <fstream>
#include "diophantine.h"
#include "defs.h"

using namespace std;
using namespace boost;

void Filter::read_filters(){
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        int8_t rhs;
        vector<uint8_t> lhs;

        {

            auto pair = split(string(line), '|');
            auto a = pair[0];
            auto lhs_s = split(string(a), ',');

            rhs = stoi(pair[1]);


            if (rhs>n_cubes) continue;
            vector<int> new_filter;
            new_filter.reserve(lhs_s.size());
            for(auto & i : lhs_s) {
                new_filter.push_back(stoi(i)-1);
            }

            filters[rhs-1].emplace_back(new_filter);
            keys.emplace_back(rhs-1);
        }
    }
    infile.close();

    std::sort(keys.begin(), keys.end()); // {1 1 2 3 4 4 5}

    auto last = std::unique(keys.begin(), keys.end());
    // v now holds {1 2 3 4 5 x x}, where 'x' is indeterminate
    keys.erase(last, keys.end());
}

bool Filter::find(const vector<int>& pile) {
    for(int i:keys){
        if (pile[i]==false) {
            auto v = filters.find(i);
            if (v!=filters.end()) {
                for (const std::vector<int>& filter : v->second) {

                    bool reject=true;
                    for (int val: filter){
                        reject &= (pile[val]);
                    }

                    if (reject) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
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
//int main() {
////    1,6,8|9
//    auto f = Filter("/Users/rvilim/repos/piles/cubes_8", 63);
////    vector<int> a = {1, 0, 0, 0, 0, 1, 0, 1, 0};
////    cout << f.find(a) << endl;
//
//    std::ifstream infile("/Users/rvilim/repos/piles/o");
//    string line;
//    while (getline(infile, line)) {
//        vector<int> l;
//        for (auto &ch: line) {
//            l.emplace_back(ch - '0');
//        }
//        if (!f.find(l)){
//            cout<<line<<endl;
//        }
//    }
//}