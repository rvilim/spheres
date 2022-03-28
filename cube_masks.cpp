#include <cstdlib>
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

vector<bool> cubes;
vector<int> combination;

void print(const vector<int>& v, int result) {
    for(int i=0;i<combination.size();i++) {
        cout<< combination[i];
        if (i!=combination.size()-1) cout<<",";
    }

    cout<<"|"<<result<<endl;
}

int sum_cube(vector<int> combination){
    int s=0;

    for(int i : combination){
        s+=pow(i,3);
    }
    return s;
}

void go(int offset, int k) {
    if (k == 0) {
        auto result = sum_cube(combination);
        if ((result<cubes.size()) && (cubes[result])){
            print(combination, result);
        };

        return;
    }
    for (int i = offset; i <= cubes.size() - k; ++i) {
        combination.push_back(cubes[i]);
        go(i+1, k-1);
        combination.pop_back();
    }
}

int main(int argc,char *argv[]){

    char* p;
    unsigned int max_cube = strtol(argv[1], &p, 10);
    if (*p != '\0') cout<<"Error";

    unsigned int max_group = strtol(argv[2], &p, 10);
    if (*p != '\0') cout<<"Error";

    uint max_size = pow(max_cube+1, 3);
    cubes.resize(max_size, false);;
    for (int i = 1; i <max_size; ++i) { cubes[i*i*i]=true; }

    for(int group_size=3;group_size<=max_group;group_size++){
        go(0, group_size);
    }
}

