cmake_minimum_required(VERSION 3.21)
project(spheres)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_executable(spheres main.cpp src/piles.cpp src/piles.h)