# Force compiler selection before project()
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang" CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++" CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.15)
project(PythonBindings CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Homebrew LLVM/Clang
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")

# Find Python
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Fetch pybind11
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG        v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# Set OpenMP flags for macOS BEFORE finding OpenMP
if(APPLE)
    # Add OpenMP directory to include path
    include_directories("/opt/homebrew/opt/libomp/include")
    
    # Set OpenMP flags
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
    
    # Link directories for OpenMP
    link_directories("/opt/homebrew/opt/libomp/lib")
endif()

# Now find OpenMP
find_package(OpenMP REQUIRED)

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")

# Create the piles module
pybind11_add_module(piles 
    src/piles.cpp
    src/bitfiltertree.cpp
)
target_compile_definitions(piles PRIVATE WITH_PYTHON)
target_link_libraries(piles PRIVATE OpenMP::OpenMP_CXX)

# Create the bitfiltertree module
pybind11_add_module(bitfiltertree 
    src/bitfiltertree.cpp
)
target_compile_definitions(bitfiltertree PRIVATE WITH_PYTHON)
target_link_libraries(bitfiltertree PRIVATE OpenMP::OpenMP_CXX)
