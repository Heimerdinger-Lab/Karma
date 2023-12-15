# 全局采用c++20
set(CMAKE_CXX_STANDARD 20)
set(VERSION "1.0")
# set(CMAKE_CXX_FLAGS "-fcoroutines")

# target
set(KARMA "karma")
set(KARMA_RAFT "karma-raft")
set(KARMA_CLIENT "karma-client")
set(KARMA_SERVICE "karma-service")
set(KARMA_SESSION "karma-session")
set(KARMA_TRANSPORT "karma-transport")
set(KARMA_UTIL "karma-util")

# co_context
set(CO_CONTEXT "co_context")

# test
set(TEST_KARMA_SERVICE test-karma-service)
set(TEST_KARMA_RAFT test-karma-raft)
set(KARMA_SERVICE_TESTS ON)
set(KARMA_RAFT_TESTS ON)