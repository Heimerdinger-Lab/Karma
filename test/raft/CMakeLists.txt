add_executable(test_raft fsm_test.cc)
target_link_libraries(test_raft Boost::unit_test_framework)
add_test(karma_test_raft test_raft)
