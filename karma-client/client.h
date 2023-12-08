#pragma once 
#include "karma-session/request_session_manager.h"
class client {
public:
    void heartbeat() {

    };
private:
    session::request_session_manager m_session_manager;
};