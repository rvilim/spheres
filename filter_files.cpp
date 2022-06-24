#include<string>
#include <fstream>
#include <iostream>
#include <vector>
#include "blockingconcurrentqueue.h"
#include <sstream>
#include <boost/dynamic_bitset.hpp>


using namespace moodycamel;
using namespace std;
using namespace boost;

BlockingConcurrentQueue<dynamic_bitset<>> input_queue;

struct file_info {
    unsigned int n_records, n_piles, n_cubes, read_progress, int_size_bytes;
};

file_info read_pile(std::string filename, int pile_num) {
    unsigned int n_records, n_piles, n_cubes, read_progress, int_size_bytes;

    std::ifstream in(filename, std::ios::binary);

    in.read(reinterpret_cast<char*>(&n_records), sizeof(n_records));
    in.read(reinterpret_cast<char*>(&n_piles), sizeof(n_piles));
    in.read(reinterpret_cast<char*>(&n_cubes), sizeof(n_cubes));
    in.read(reinterpret_cast<char*>(&pile_num), sizeof(pile_num));
    in.read(reinterpret_cast<char*>(&int_size_bytes), sizeof(int_size_bytes));

    file_info info = {n_records, n_piles, n_cubes, read_progress, int_size_bytes};

    vector<int_fast8_t> pile(n_cubes, 0);

    while (true){
        if (in.read(reinterpret_cast<char *>(&pile[0]), int_size_bytes * n_cubes)) {
            dynamic_bitset<> pile_bitset(n_cubes);

            for(int i=0;i<pile.size();i++){
                if(pile[i]==pile_num){
                    pile_bitset[i]=1;
                }
            }
            input_queue.enqueue(pile_bitset);
        } else {
            return info;
        }
    }
}


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

vector<vector<dynamic_bitset<>>> read_filters(string filename, file_info info){
    vector<vector<dynamic_bitset<>>> filters(100);
    std::ifstream infile(filename);
    string line;

    while (getline(infile, line)){
        dynamic_bitset<> lhs(info.n_cubes);
        int8_t rhs;

        {
            auto pair = split(string(line), '|');
            auto lhs_s = split(pair[0], ',');

            rhs = stoi(pair[1]);

            if (rhs>info.n_cubes) continue;

            for(auto & i : lhs_s){
                if (stoi(i)>info.n_cubes) continue;
                lhs[stoi(i)-1]=1;
            }
        }
        filters[rhs].push_back(lhs);
    }
    infile.close();

    return filters;
}

bool filter_pile(dynamic_bitset<> &pile, vector<vector<dynamic_bitset<>>> &filters){
    for(int i=0;i<pile.size();i++){
        if (!pile[i]){
            for (auto & filter : filters[i]){
                if ((filter & pile) == filter){
//                    cout<<"--"<<endl;
//                    cout<<i<<endl;
//                    cout<<filter<<endl;
//                    cout<<(filter & pile)<<endl;
//                    cout<<pile<<endl;
                    return true;
                }
            }
        }
    }
    return false;
}


int main(){
    auto info = read_pile("/Users/rvilim/repos/piles/11_65_1", 2);

    auto filters = read_filters("/Users/rvilim/repos/piles/cubes_8", info);

    dynamic_bitset<> pile;

    int n_filtered=0;
    int n_not_filtered =0;
    while (true){
        auto ret = input_queue.try_dequeue(pile);
        if (filter_pile(pile, filters)){
            n_filtered++;

            if (n_filtered%1000==0){
                cout<<"Filtered: "<<n_filtered<<", Not Filtered: "<<n_not_filtered<<", Remaining: "<<input_queue.size_approx()<<endl;
            }
        }else{
            n_not_filtered++;
        }

        if(!ret){
            break;
        }
    }

    cout<<info.n_records<<endl;

    return 0;
}