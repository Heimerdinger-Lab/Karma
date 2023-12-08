#pragma once 
#include <co_context/all.hpp>
#include "transport/connection.h"
#include "error.h"
#include "frame.h"
class response_session {
public:
    response_session();
    void process() {
        co_context::co_spawn(process0());
    }
    co_context::task<> process0() {
        transport::connection connection(std::move(m_socket), m_inet_address);
        // read loop
        while (true) {
            auto res = co_await connection.read_frame();
            // 弄一个工厂模式
            // fsm，和client都是通过取模
            // 
        }

        // write loop
        // 不需要write loop
        // 因为我是单向的
    }
private:    
    co_context::inet_address m_inet_address;
    co_context::socket m_socket;
};  