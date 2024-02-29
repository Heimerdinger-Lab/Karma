#pragma once
#include "karma-raft/raft.hh"
#include <cstdint>
#include <map>
#include <vector>
namespace service {
struct config {
    uint64_t m_id;
    std::string m_listen_ip;
    uint16_t m_listen_port;
    std::string m_store_path;
    uint64_t m_count;
    std::map<uint64_t, std::string> m_members;
    
};
};