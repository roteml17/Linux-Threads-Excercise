# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(BlockChainProject VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find zlib
find_package(ZLIB REQUIRED)

# Add an executable
add_executable(blockchain ex02.cpp blockChain.cpp)

# Include directories
target_include_directories(blockchain PUBLIC "${PROJECT_SOURCE_DIR}")

# Link zlib and pthread libraries
target_link_libraries(blockchain ZLIB::ZLIB pthread)

