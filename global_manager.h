#pragma once 
#include "karma-session/request_session_manager.h"
#include <vector>
class global_request_session_manager {
public:
    static global_request_session_manager &get_instance() {
        static global_request_session_manager instance_;
        return instance_;
    }
    session::request_session_manager get_request_session_manager() {

    }
private:
    std::vector<session::request_session_manager> m_managers; 
};

class global_raft_group_manager {
public:

private:
    
};