#pragma once
#include "karma-raft/common.h"
#include <string>
#include <variant>
namespace client {

class ping_pong_request_header {
public:
    raft::server_id m_start;
    raft::group_id m_group_id;
    // 
    std::string m_str;
};
class ping_pong_reply_header {
public:
    raft::server_id m_start;
    raft::group_id m_group_id;
    // 
    std::string m_str;
};


using header = std::variant<ping_pong_request_header, ping_pong_reply_header>;
};