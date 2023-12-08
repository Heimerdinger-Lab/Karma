file(GLOB_RECURSE UTIL_HEADERS "*.h")
file(GLOB_RECURSE UTIL_SRCS "*.cc")

message(STATUS "UTIL_HEADERS: ${UTIL_HEADERS}")
message(STATUS "UTIL_SRCS: ${UTIL_SRCS}")

add_library(${KARMA_UTIL} ${UTIL_HEADERS} ${UTIL_SRCS})
target_include_directories(${KARMA_UTIL} PRIVATE . ${CMAKE_SOURCE_DIR})
target_link_libraries(${KARMA_UTIL})
