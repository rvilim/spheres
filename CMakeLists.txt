cmake_minimum_required(VERSION 3.21)
project(piles)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


find_package(Boost 1.78.0 REQUIRED)


if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    add_executable(piles main.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)
    add_executable(bench bench.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)

    add_executable(diophantine diophantine.cpp diophantine.h defs.h)
    add_executable(test_diophantine test_diophantine.cpp diophantine.cpp diophantine.h defs.h)

#    add_executable(tests defs.h piles.cpp piles.h test_piles.cpp)

    target_link_libraries (diophantine ${Boost_LIBRARIES})
    target_link_libraries (test_diophantine ${Boost_LIBRARIES})

endif()