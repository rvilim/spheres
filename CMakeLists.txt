cmake_minimum_required(VERSION 3.21)
project(piles)

set(CMAKE_CXX_STANDARD 23)

add_executable(piles main.cpp defs.h piles.cpp piles.h ThreadSafeQueue.h)
add_executable(tests defs.h piles.cpp piles.h test_piles.cpp)
