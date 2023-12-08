file(GLOB_RECURSE SERVICE_HEADERS "*.h")
file(GLOB_RECURSE SERVICE_SRCS "*.cc")

message(STATUS "SERVICE_HEADERS: ${SERVICE_HEADERS}")
message(STATUS "SERVICE_SRCS: ${SERVICE_SRCS}")

add_library(${KARMA_SERVICE} ${SERVICE_HEADERS} ${SERVICE_SRCS})
target_include_directories(${KARMA_SERVICE} PRIVATE . ${CMAKE_SOURCE_DIR})

target_link_libraries(${KARMA_SERVICE})

if(KARMA_SERVICE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
