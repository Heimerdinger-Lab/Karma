# 全局采用c++20
set(CMAKE_CXX_STANDARD 20)
set(VERSION "1.0")
set(CMAKE_CXX_FLAGS "-fcoroutines")

# target
set(KARMA "karma")
set(KARMA_RAFT "karma-raft")
# set(KARMA_RAFT "karma-raft")
set(KARMA_CLIENT "karma-client")
set(KARMA_SERVICE "karma-service")
set(KARMA_SESSION "karma-session")
set(KARMA_TRANSPORT "karma-transport")
set(KARMA_UTIL "karma-util")
set(KARMA_STORE "karma-store")
set(KARMA_CLI "karma-cli")
# 
set(KARMA_BENCH "benchmark")

# co_context
set(CO_CONTEXT "co_context")

# test
set(TEST_KARMA_SERVICE test-karma-service)
# set(TEST_KARMA_RAFT test-karma-raft)
set(TEST_KARMA_RAFT test-karma-raft)
set(TEST_KARMA_TRANSPORT test-karma-transport)
set(TEST_KARMA_CLIENT test-karma-client)
set(TEST_KARMA_3RD_PART 3rd-part)
set(TEST_KARMA_STORE test-karma-store)

set(KARMA_SERVICE_TESTS OFF)
set(KARMA_RAFT_TESTS OFF)
set(KARMA_TRANSPORT_TESTS OFF)
set(KARMA_CLIENT_TESTS OFF)
set(KARMA_3RD_TESTS OFF)
set(KARMA_STORE_TESTS OFF)