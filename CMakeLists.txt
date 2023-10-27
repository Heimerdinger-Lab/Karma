cmake_minimum_required(VERSION 3.20)

set (CMAKE_C_COMPILER "/usr/bin/gcc")
set (CMAKE_CXX_COMPILER "/usr/bin/g++")

project(karma)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_subdirectory(co_context)

add_executable(karma main.cc)

target_include_directories(karma PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/co_context/include")
target_link_libraries(karma co_context)
