file(GLOB_RECURSE RAFT_HEADERS "*.hh")
file(GLOB_RECURSE RAFT_SRCS "*.cc")

message(STATUS "RAFT_HEADERS: ${RAFT_HEADERS}")
message(STATUS "RAFT_SRCS: ${RAFT_SRCS}")

add_library(${KARMA_RAFT} ${RAFT_HEADERS} ${RAFT_SRCS})
target_include_directories(${KARMA_RAFT} PRIVATE . ${CMAKE_SOURCE_DIR} ${CO_CONTEXT_DIR})
target_link_libraries(${KARMA_RAFT} ${CO_CONTEXT} fmt)
