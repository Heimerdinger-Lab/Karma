file(GLOB_RECURSE STORE_HEADERS "*.h")
file(GLOB_RECURSE STORE_SRCS "*.cc")

message(STATUS "STORE_HEADERS: ${STORE_HEADERS}")
message(STATUS "STORE_SRCS: ${STORE_SRCS}")
# find_library(LIBURING_LIBRARY NAMES uring)
add_library(${KARMA_STORE} ${STORE_HEADERS} ${STORE_SRCS})
target_include_directories(${KARMA_STORE} PRIVATE . ${CMAKE_SOURCE_DIR})
target_link_libraries(${KARMA_STORE} ${CO_CONTEXT} ${KARMA_UTIL} uring) 
