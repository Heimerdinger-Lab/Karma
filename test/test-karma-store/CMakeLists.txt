file(GLOB_RECURSE SOURCES "*.cc")

add_executable(${TEST_KARMA_STORE} ${SOURCES})

target_include_directories(${TEST_KARMA_STORE} PRIVATE . ${CMAKE_SOURCE_DIR})

target_link_libraries(${TEST_KARMA_STORE} ${KARMA_UTIL} ${KARMA_STORE} Boost::unit_test_framework Boost::log uring)

add_test(NAME ${TEST_KARMA_STORE} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_KARMA_STORE})
