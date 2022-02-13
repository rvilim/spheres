#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <numeric>
#include <array>

#include "piles.h"
#include "defs.h"

using namespace std;
int num=0;

const int sums[100] = {1, 9, 36, 100, 225, 441, 784, 1296, 2025, 3025, 4356, 6084, 8281, 11025, 14400, 18496, 23409, 29241, 36100, 44100, 53361, 64009, 76176, 90000, 105625, 123201, 142884, 164836, 189225, 216225, 246016, 278784, 314721, 354025, 396900, 443556, 494209, 549081, 608400, 672400, 741321, 815409, 894916, 980100, 1071225, 1168561, 1272384, 1382976, 1500625, 1625625, 1758276, 1898884, 2047761, 2205225, 2371600, 2547216, 2732409, 2927521, 3132900, 3348900, 3575881, 3814209, 4064256, 4326400, 4601025, 4888521, 5189284, 5503716, 5832225, 6175225, 6533136, 6906384, 7295401, 7700625, 8122500, 8561476, 9018009, 9492561, 9985600, 10497600, 11029041, 11580409, 12152196, 12744900, 13359025, 13995081, 14653584, 15335056, 16040025, 16769025, 17522596, 18301284, 19105641, 19936225, 20793600, 21678336, 22591009, 23532201, 24502500, 25502500};
const int cubes[100] = {1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047, 262144, 274625, 287496, 300763, 314432, 328509, 343000, 357911, 373248, 389017, 405224, 421875, 438976, 456533, 474552, 493039, 512000, 531441, 551368, 571787, 592704, 614125, 636056, 658503, 681472, 704969, 729000, 753571, 778688, 804357, 830584, 857375, 884736, 912673, 941192, 970299, 1000000};

int sum_pile(vector<bool> pile){
    int s=0;

    auto n_cubes=pile.size();
    for(int pos=0; pos<n_cubes; pos++){
        s+=pile[pos]*cubes[pos];
    }

    return s;
}

void print_pile(vector<bool> pile){
    for(auto p : pile){
        cout<<p;
    }
//    cout<<" -> "<<sum_pile(pile)<<endl;
    cout<<endl;
}

void make_pile(int target, int remaining, int pos, vector<bool> &pile, vector<bool> &disallowed, ThreadsafeQueue<vector<bool>> &accumulator, int n_piles, int n_cubes){

    if (remaining<target){ // If there is no way we can construct a pile from the remaining cubes, return
        return;
    }

    if ((not disallowed[pos]) && cubes[pos]==target) { // Success! we have a pile
        pile[pos] = true;
        num += 1;
        accumulator.push(pile);
        pile[pos] = false;
    }

    if (pos==0){ // Can't find a pile
        return;
    }else{ // Hope is still alive

        if (disallowed[pos]){ // If the one we are on is disallowed, just skip it.
            make_pile(target, remaining, pos-1, pile, disallowed, accumulator, n_piles, n_cubes);
        } else {
            // Call the function again with the bit in question both set and unset

            if (target> cubes[pos]){
                pile[pos] = true;
                make_pile(target - cubes[pos], remaining - cubes[pos], pos - 1, pile, disallowed, accumulator, n_piles, n_cubes);
                pile[pos] = false;
            }
            make_pile(target, remaining, pos - 1, pile, disallowed, accumulator, n_piles, n_cubes);
        }
    }
}