#include <iostream>
#include <bitset>
#include <vector>

int main() {
    const int numBits = 23;
    const int numCombinations = 1 << numBits; // 2^23

    for (int i = 0; i < numCombinations; ++i) {
        int sum = 0;
        std::bitset<numBits> bits(i);

        // Gather set bit positions
        std::vector<int> setBitPositions;
        for (int j = 0; j < numBits; ++j) {
            if (bits.test(j)) {
//                setBitPositions.push_back(j);
                sum += (j + 1) * (j + 1) * (j + 1);
            }
        }
        if ((sum == 25392) && bits.test(numBits-1)) {
            std::cout << bits << std::endl;
        }
    }
//
//        // Print bit string
//        std::cout << bits.to_string() << " - Set bit positions: ";
//
//        // Print set bit positions
//        for(size_t j = 0; j < setBitPositions.size(); ++j) {
//            std::cout << setBitPositions[j];
//            if(j != setBitPositions.size() - 1) {
//                std::cout << ", ";
//            }
//        }
//        std::cout << '\n';
//    }

    return 0;
}
