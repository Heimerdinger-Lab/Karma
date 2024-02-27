#pragma once
#include "co_context/all.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "frame.h"
#include "error.h"
#include "write_task.h"
namespace transport {
class connection {
public:
    connection(std::shared_ptr<co_context::socket> socket, std::string addr, uint8_t port) 
    : m_socket(socket)
    , m_inet_address(addr, port) {
        co_context::co_spawn(loop());
    }
    ~connection() = default;
    co_context::task<std::optional<connection_error>> write(std::shared_ptr<frame> f) {
        auto encoded = f->encode();
        auto total = encoded.length();
        auto ret = co_await m_socket->send(encoded);
        std::cout << " i have write " << ret  << ", total " << total << std::endl;
        if (ret < total) {
            co_return connection_error::network_error;
        }
        co_return std::nullopt;
    };

    std::optional<std::shared_ptr<frame>> parse_frame() {
        if (m_buffer.size() == 0) {
            return std::nullopt;
        }
        try {
            frame::check(std::span<char>(m_buffer));
            return frame::parse(std::span<char>(m_buffer));
        } catch (frame_error e) {
            std::cout << "fe" << std::endl;
            return std::nullopt;
        }
    };

    co_context::task<std::shared_ptr<frame>> read_frame() {
        while(true) {
            auto res = parse_frame();
            if (res.has_value()) {
                auto f = res.value();
                auto siz = f->size();
                std::cout << " i have read a frame: " << siz << std::endl;
                m_buffer.erase(0, siz);
                co_return f;
            } else {
                std::cout << "no value" << std::endl;
            }
            // if (m_buffer.capacity() < 4096) {
            //     m_buffer.reserve(4096);
            // }
            char buf[128] = {0};
            std::cout << "to wait: " << m_socket.use_count() << std::endl;
            auto read_size = co_await m_socket->recv(buf);
            std::cout << "end wait, read_size: "  << read_size << std::endl;
            m_buffer.insert(m_buffer.end(), buf, buf + read_size);
            if (read_size == 0) {
                std::cout << "size = 0" << std::endl;
                // throw frame_error::connection_reset();
            } 
        };
    };
    co_context::task<std::optional<connection_error>> write_frame(std::shared_ptr<frame> f) {
        std::shared_ptr<co_context::channel<std::optional<connection_error>, 0>> observer = std::make_shared<co_context::channel<std::optional<connection_error>, 0>>();
        write_task task;
        task.m_frame = f;
        task.m_observer = observer;
        co_await m_write_task_chan.release(task);
        auto result = co_await observer->acquire();
        co_return result;
    };
private:
    co_context::task<void> loop() {
        while (1) {
            auto task = co_await m_write_task_chan.acquire();
            std::cout << "got a task" << std::endl;
            auto ret = co_await write(task.m_frame);

            co_await task.m_observer->release(ret);
            
        };
    }
    co_context::inet_address m_inet_address;
    std::shared_ptr<co_context::socket> m_socket;
    co_context::channel<write_task> m_write_task_chan;
    std::string m_buffer;
};
}