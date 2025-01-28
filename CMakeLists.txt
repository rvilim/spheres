cmake_minimum_required(VERSION 3.21)
project(spheres)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(OpenMP_CXX_FLAGS "-fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")


add_executable(spheres main.cpp src/piles.cpp src/piles.h)
add_executable(test_classify src/test_classify.cpp src/bitfiltertree.cpp src/bitfiltertree.h)
