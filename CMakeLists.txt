cmake_minimum_required(VERSION 3.20)

set (CMAKE_C_COMPILER "/usr/bin/clang")
set (CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(karma)

find_package(Boost REQUIRED COMPONENTS system thread)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_executable(karma main.cc)
target_link_libraries(karma Boost::system)