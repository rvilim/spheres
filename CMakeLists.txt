cmake_minimum_required(VERSION 3.15)
project(PythonBindings CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only set specific compiler on macOS
if(APPLE)
    # Force compiler selection before project()
    set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++" CACHE STRING "" FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set OpenMP flags for macOS
if(APPLE)
    include_directories("/opt/homebrew/opt/libomp/include")
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
    link_directories("/opt/homebrew/opt/libomp/lib")
endif()

# Find OpenMP
find_package(OpenMP REQUIRED)

# Find zstd package
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED libzstd)

# Add Arrow and Parquet dependencies
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

if(APPLE)
    include_directories(/opt/homebrew/Cellar/rocksdb/9.10.0/include/)
    include_directories(/opt/homebrew/Cellar/zstd/1.5.6/include/)
    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/Cellar/rocksdb/9.10.0/lib/)
    link_directories(/opt/homebrew/Cellar/zstd/1.5.6/lib/)
    link_directories(/opt/homebrew/lib)
endif()

include_directories(/usr/local/include/)
link_directories(/usr/local/lib/)

# Create the core library that doesn't depend on Python/nanobind
add_library(core_lib
    src/bitfiltertree.cpp
    src/piles.cpp
)
target_link_libraries(core_lib PRIVATE OpenMP::OpenMP_CXX)

# Only build Python bindings if Python is available
find_package(Python COMPONENTS Interpreter Development)
if(Python_FOUND)
    # Detect the installed nanobind package and import it into CMake
    #execute_process(
    #    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    #    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT
    #    RESULT_VARIABLE NANOBIND_RESULT
    #)
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanobind" AND NANOBIND_RESULT EQUAL 0)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanobind)
        list(APPEND CMAKE_PREFIX_PATH (${CMAKE_CURRENT_SOURCE_DIR}/ext/nanobind))
        find_package(nanobind CONFIG REQUIRED)

        # Create the Python modules using the core library
        nanobind_add_module(piles src/piles.cpp)
        target_link_libraries(piles PRIVATE core_lib OpenMP::OpenMP_CXX)
        target_compile_definitions(piles PRIVATE NB_MODULE)

        nanobind_add_module(bitfiltertree src/bitfiltertree.cpp)
        target_link_libraries(bitfiltertree PRIVATE core_lib OpenMP::OpenMP_CXX)
        target_compile_definitions(bitfiltertree PRIVATE NB_MODULE)
    endif()
endif()

# Create the bench executable using the core library
add_executable(bench src/bench.cpp)
target_link_libraries(bench PRIVATE core_lib OpenMP::OpenMP_CXX)

# Create the solve executable using the core library
add_executable(solve 
    src/solve.cpp 
    src/duplicatechecker.cpp
    src/blockingconcurrentqueue.h 
    src/concurrentqueue.h 
    src/lightweightsemaphore.h 
    src/duplicatechecker.h
)

target_link_libraries(solve PRIVATE 
    core_lib 
    OpenMP::OpenMP_CXX 
    rocksdb
    zstd
    arrow
    parquet
    $<$<NOT:$<PLATFORM_ID:Darwin>>:snappy>
    $<$<NOT:$<PLATFORM_ID:Darwin>>:z>
    $<$<NOT:$<PLATFORM_ID:Darwin>>:lz4>
    $<$<NOT:$<PLATFORM_ID:Darwin>>:bz2>
)
