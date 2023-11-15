add_library(raft)
target_sources(raft PRIVATE server.cc)
#   PRIVATE
#     fsm.cc
#     log.cc
#     raft.cc

#     tracker.cc)

target_include_directories(raft PUBLIC ${CMAKE_SOURCE_DIR})

# target_link_libraries(raft PUBLIC)
