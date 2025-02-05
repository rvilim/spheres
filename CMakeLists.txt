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


if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Find Python first
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Detect the installed nanobind package and import it into CMake
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)

# Set OpenMP flags for macOS BEFORE finding OpenMP
if(APPLE)
    # Add OpenMP directory to include path
    include_directories("/opt/homebrew/opt/libomp/includse")
    
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

# Create the piles module
nanobind_add_module(piles 
    src/piles.cpp
    src/bitfiltertree.cpp
)

# Link OpenMP to piles
target_link_libraries(piles PRIVATE OpenMP::OpenMP_CXX)

# Create the bitfiltertree module
nanobind_add_module(bitfiltertree 
    src/bitfiltertree.cpp
)
target_link_libraries(bitfiltertree PRIVATE OpenMP::OpenMP_CXX)