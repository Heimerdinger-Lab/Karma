#pragma once
#include "transport/connection.h"
class request_session {
    request_session(); 
        // co_spawn()
        // read write task
    
public:
    void write() {
        // 构造一个task
        // 丢给spawn出来的write线程
        // m_connection.write_frame();
    }   
private:
    transport::connection m_connection;
};