cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(compiler_settings)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "VERSION: ${VERSION}")
message(STATUS "KARMA: ${KARMA}")

project(${KARMA} VERSION ${VERSION})

include(CTest)
include(third_party_package)
include(local_package)
# test
add_subdirectory(test)

#clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${KARMA}  main.cc)
target_include_directories(${KARMA} PRIVATE . ${CO_CONTEXT_DIR} ${CMAKE_SOURCE_DIR})
# target_link_libraries(${KARMA} ${CO_CONTEXT} ${KARMA_CLIENT} ${KARMA_RAFT} ${KARMA_SERVICE} ${KARMA_SESSION} ${KARMA_TRANSPORT} ${KARMA_UTIL}) 
target_link_libraries(${KARMA} ${CO_CONTEXT} ${KARMA_TRANSPORT}) 



# add_executable(${KARMA} tool/async.cc)
# target_include_directories(${KARMA} PRIVATE . ${CO_CONTEXT_DIR} ${CMAKE_SOURCE_DIR})
# target_link_libraries(${KARMA} ${CO_CONTEXT}) 
