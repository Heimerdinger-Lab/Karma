#pragma once
#include <string>
struct write_options {
    bool sync = false;
};
struct open_options {
    int queue_depth = 32768;
    int sqpoll_cpu = 0;
    std::string path;
};