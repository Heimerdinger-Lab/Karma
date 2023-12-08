#pragma once
#include "co_context/all.hpp"
#include <vector>
#include "frame.h"
#include "error.h"
#include "write_task.h"
// #include "util/common.h"
#include "util/result.h"
namespace transport {
class connection {
public:
    connection(co_context::socket&& socket, std::string addr, uint8_t port) 
    : m_socket(std::move(socket))
    , m_inet_address(addr, port) {}
    
    connection(co_context::socket&& socket, co_context::inet_address addr) 
    : m_socket(std::move(socket))
    , m_inet_address(addr) {}
    co_context::task<> write() {
        
    };

    std::optional<frame> parse_frame() {
        try {
            return frame::parse(std::span<char>(m_buffer));
        } catch (frame_error e) {
            return std::nullopt;
        }
    };

    // 并发安全：外部是一个循环，所以不会有并发调用
    co_context::task<frame> read_frame() {
        while(true) {
            auto res = parse_frame();
            if (res.has_value()) {
                co_return res.value();
            }
            if (m_buffer.capacity() < 4096) {
                m_buffer.reserve(4096);
            }
            auto read_size = co_await m_socket.recv(std::span(m_buffer).subspan(m_buffer.size()), 0);
            if (read_size == 0) {
                throw frame_error::connection_reset();
            } 
        };
    };
    // 并发安全：外部是个循环，所以不会有并发安全
    co_context::task<> write_frame(const frame& f) {
        // co_context::channel<>
        co_context::mutex mtx;
        mtx.lock();
        write_task task;
        task.m_cv.wait(mtx);

    };
private:
    co_context::inet_address m_inet_address;
    co_context::socket m_socket;
    co_context::channel<std::string> write_task_chan;
    std::string m_buffer;
};
}