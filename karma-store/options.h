#pragma once
#include <cstdint>
#include <string>
struct write_options {};
struct open_options {
    uint32_t queue_depth = 32768;
    uint32_t sqpoll_cpu = 0;
    uint32_t segment_file_size = 1048576;
    uint32_t preallocated_count = 2;
    std::string path;
};