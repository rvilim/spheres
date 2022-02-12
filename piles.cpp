#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <numeric>


#include "piles.h"
#include "defs.h"

using namespace std;

const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

int sum_pile(vector<bool> pile){
    int s=0;

    for(int i=0; i<pile.size(); i++){
        s+=pile[i]*cubes[i];
    }

    return s;
}

void print(vector<vector<int>> piles, int n_cubes){
//    for(auto & p : piles[0]){
//        std::cout<<p<<" ";
//    }
    // Make a string of all zeros, then replace the index corresponding to the cube root of each number with a 1
    for (auto & pile : piles) {
        string line = string(n_cubes,'0');

        for(auto & p : pile){
            line[cbrt(p)-1]='1';
        }
        cout<<line<<" "<<accumulate(pile.begin(), pile.end(), 0)<<endl;
    }
    cout<<endl;
}

vector<vector<int>> place(vector<vector<int>> &piles, vector<int> &remaining, int pos, int n_cubes){
    int n_piles = piles.size();

    for(int p=0; p<n_piles; p++){
        remaining[p]-=cubes[pos];

        if (remaining[p] >= 0){
            if (pos==0){

                for(int i=0;i<piles.size();i++){
                    cout<<"Remaining in pile "<<i<<": "<<remaining[i]<<endl;
                }

                piles[p].emplace_back(cubes[pos]);

                return piles;
            }else{
                if (remaining[p]>=0) {
                    piles[p].emplace_back(cubes[pos]);

                    auto ret = place(piles, remaining, pos - 1, n_cubes);

                    if (ret[0].size()!=0){
                        return ret;
                    }
                    piles[p].pop_back();
                }
            }
        }

        remaining[p]+=cubes[pos];
    }
    return vector<vector<int>>{{}};
}

vector<vector<int>> init_distribution(int n_piles, int n_cubes){
    int target = sums[n_cubes-1]/n_piles;

    vector<vector<int>> piles(1);
    piles[0].push_back((n_cubes*n_cubes*n_cubes));

    for (int pile_num=1; pile_num<n_piles; pile_num++){
        // The condition here is that we can place things without loss of generality
        // as long as two adjacent cubes add up to greater than the target. As soon
        // as that's not true the smaller could either go in a new pile, or double up
        // with an already placed pile and we have to leave it to the main solver to
        // figure tha one out.
        if (cubes[n_cubes-pile_num]+cubes[n_cubes-pile_num-1]>target){
            int c = (n_cubes-pile_num);
            piles.push_back({c*c*c});
        }
    }

    // Once we have placed all the cubes we can place, we need to pad
    // out our piles to n_piles with empty piles
    while (piles.size()<n_piles){
        piles.emplace_back(vector<int>{});
    }

    for(auto & pile : piles){
        pile.reserve(n_cubes/n_piles);
    }

    return piles;
}

vector<int> init_remaining(vector<vector<int>> piles, int n_cubes){
    // Given a vector of piles, calculate the remaining amount that the piles needs
    auto n_piles = piles.size();

    vector<int> remaining={};
    for(auto & pile : piles){
        if (pile.size()!=0) {
            remaining.emplace_back(sums[n_cubes - 1] / n_piles - pile[0]);
        }else{
            remaining.emplace_back(sums[n_cubes - 1] / n_piles);
        }
    }
    return remaining;
}

int init_pos(vector<vector<int>> piles){
    int pos=0;
    auto it = piles.rbegin();

    while (it != piles.rend()){
        if ((*it).size()!=0){
            pos = cbrt((*it)[0])-2;
            break;
        }
        it++;
    }

    return pos;
}
