file(GLOB_RECURSE SOURCES "*.cc")

add_executable(${TEST_KARMA_RAFT} ${SOURCES})

target_include_directories(${TEST_KARMA_RAFT} PRIVATE . ${CMAKE_SOURCE_DIR})

target_link_libraries(${TEST_KARMA_RAFT} ${KARMA_RAFT} Boost::unit_test_framework)

add_test(NAME ${TEST_KARMA_RAFT} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_KARMA_RAFT})
