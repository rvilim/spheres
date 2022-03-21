cmake_minimum_required(VERSION 3.21)
project(piles)
set(Boost_USE_STATIC_LIBS   ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


#find_package(Boost REQUIRED)
find_package(Boost 1.78.0 REQUIRED)

#include_directories(${Boost_INCLUDE_DIRS})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})



#    add_executable(piles main.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)
#    add_executable(bench bench.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)
    add_executable(piles_files main_files.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)
    add_executable(bench bench.cpp defs.h piles.cpp piles.h blockingconcurrentqueue.h lightweightsemaphore.h concurrentqueue.h)

    #    target_include_directories( piles_files PRIVATE ${Boost_INCLUDE_DIR})
#    add_executable(tests defs.h piles.cpp piles.h test_piles.cpp)
#    target_link_libraries(piles_files Boost::program_options)
    target_link_libraries (piles_files ${Boost_LIBRARIES})
endif()

#target_link_libraries(piles_files Boost::program_options Boost::dynamic_bitset)