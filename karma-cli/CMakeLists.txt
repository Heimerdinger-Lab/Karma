file(GLOB_RECURSE CLI_HEADERS "*.h")
file(GLOB_RECURSE CLI_SRCS "*.cc")

message(STATUS "CLI_HEADERS: ${CLI_HEADERS}")
message(STATUS "CLI_SRCS: ${CLI_SRCS}")

add_executable(${KARMA_CLI} ${CLI_HEADERS} ${CLI_SRCS})
target_include_directories(${KARMA_CLI} PRIVATE . ${CMAKE_SOURCE_DIR} ${CO_CONTEXT_DIR})
target_link_libraries(${KARMA_CLI} ${CO_CONTEXT} ${KARMA_CLIENT} ${KARMA_TRANSPORT})
