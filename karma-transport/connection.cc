#include "connection.h"

#include <boost/log/trivial.hpp>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>

transport::connection::connection(std::unique_ptr<co_context::socket> socket, std::string addr,
                                  uint8_t port)
    : m_socket(std::move(socket)), m_inet_address(addr, port), m_valid(true) {
    co_context::co_spawn(loop());
}
co_context::task<std::optional<std::unique_ptr<transport::frame>>>
transport::connection::read_frame() noexcept {
    if (!valid()) {
        BOOST_LOG_TRIVIAL(error) << "Fail to read a frame from an invalid connection";
        co_return std::nullopt;
    }
    try {
        while (true) {
            auto frame_opt = frame::parse(std::span<char>(m_buffer));
            if (frame_opt.has_value()) {
                auto frame = std::move(frame_opt.value());
                m_buffer.erase(0, frame->size());
                co_return std::move(frame);
            }
            char buf[128] = {0};
            // 返回 <= 0，说明链接出错。
            auto read_size = co_await m_socket->recv(buf);
            if (read_size <= 0) {
                BOOST_LOG_TRIVIAL(error) << "Something wrong with this connection" << std::endl;
                throw std::runtime_error("The return value of receive() <= 0");
            }
            m_buffer.insert(m_buffer.end(), buf, buf + read_size);
        };
    } catch (std::runtime_error e) {
        BOOST_LOG_TRIVIAL(error) << "Runtime error reason: " << e.what()
                                 << ", connection will be reset";
    }

    m_valid = false;
    co_return std::nullopt;
};

co_context::task<bool> transport::connection::write_frame(frame& f) {
    if (!valid()) {
        BOOST_LOG_TRIVIAL(error) << "Fail to write a frame from an invalid connection";
        co_return false;
    }
    auto observer = std::make_unique<co_context::channel<bool>>();
    write_task task{.m_frame = f, .m_observer = *observer};
    co_await m_write_task_chan.release(task);
    auto result = co_await observer->acquire();
    co_return result;
};

co_context::task<bool> transport::connection::write(frame& f) {
    auto encoded = f.encode();
    auto total = encoded.length();
    auto ret = co_await m_socket->send(encoded);
    if (ret != total) {
        BOOST_LOG_TRIVIAL(error) << "Write fail, return length != frame's length" << std::endl;
        m_valid = false;
        co_return false;
    }
    co_return true;
};
co_context::task<void> transport::connection::loop() {
    BOOST_LOG_TRIVIAL(trace) << "Connection write loop started";
    while (m_valid) {
        auto task = co_await m_write_task_chan.acquire();
        if (!m_valid) {
            co_await task.m_observer.release(false);
        }
        auto ret = co_await write(task.m_frame);
        co_await task.m_observer.release(ret);
    };
}
