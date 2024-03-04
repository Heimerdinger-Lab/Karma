#pragma once
#include <cstdint>
#include <string>
struct write_options {};
struct open_options {
    uint32_t queue_depth = 32768;
    uint32_t sqpoll_cpu = 0;
    uint64_t segment_file_size = 1024 * 1024;
    uint64_t preallocated_count = 2;
    std::string path;
};