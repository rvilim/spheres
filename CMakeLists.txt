cmake_minimum_required(VERSION 3.21)
project(piles)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_executable(piles main.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)
add_executable(bench bench.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)
add_executable(piles_files main_files.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)

add_executable(tests defs.h piles.cpp piles.h test_piles.cpp)
