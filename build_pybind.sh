clang++ -O3 -Wall -shared -std=c++20 -fPIC -DWITH_PYTHON \
    $(python3 -m pybind11 --includes) \
    src/piles.cpp src/bitfiltertree.cpp \
    -undefined dynamic_lookup \
    -fopenmp \
    -o piles$(python3-config --extension-suffix)

clang++ -O3 -Wall -shared -std=c++20 -fPIC -DWITH_PYTHON \
    $(python3 -m pybind11 --includes) \
    src/bitfiltertree.cpp \
    -undefined dynamic_lookup \
    -fopenmp \
    -o bitfiltertree$(python3-config --extension-suffix)
