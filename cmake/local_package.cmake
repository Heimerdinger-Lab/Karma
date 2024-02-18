# 主要是co_context和karma自身
add_subdirectory(${KARMA_RAFT})
# add_subdirectory(${KARMA_CLIENT})
# add_subdirectory(${KARMA_SERVICE})
# add_subdirectory(${KARMA_SESSION})
# add_subdirectory(${KARMA_TRANSPORT})
add_subdirectory(${KARMA_UTIL})

# 
add_subdirectory(${CO_CONTEXT})
set(${CO_CONTEXT_DIR} "${CMAKE_SOURCE_DIR}/${CO_CONTEXT}/include" )