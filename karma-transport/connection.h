#pragma once
#include <memory>
#include <optional>

#include "co_context/all.hpp"
#include "frame.h"
#include "write_task.h"
namespace transport {
class connection {
   public:
    connection(std::unique_ptr<co_context::socket> socket, std::string addr, uint8_t port);
    ~connection() { ::close(m_socket->fd()); }
    co_context::task<std::optional<std::unique_ptr<frame>>> read_frame() noexcept;
    co_context::task<bool> write_frame(frame& f);
    bool valid() { return m_valid; }

   private:
    std::optional<std::unique_ptr<frame>> parse_frame();
    co_context::task<bool> write(frame& f);
    co_context::task<void> loop();
    co_context::inet_address m_inet_address;
    std::unique_ptr<co_context::socket> m_socket;
    co_context::channel<write_task> m_write_task_chan;
    std::string m_buffer;
    bool m_valid;
};
}  // namespace transport