#pragma once
#include <co_context/all.hpp>
#include <vector>

class connection {
    connection(co_context::socket&& socket, std::string addr, uint8_t port) 
    : m_socket(std::move(socket))
    , m_addr(addr)
    , m_port(port) {}
    
    co_context::task<> write() {
        
    };
    co_context::task<> read() {

    };

    bool parse_frame() {

    };

private:
    std::string m_addr;
    uint8_t m_port;
    co_context::socket m_socket;
    co_context::channel<std::string, 1024> chan;
    std::vector<uint8_t> m_buffer;
};