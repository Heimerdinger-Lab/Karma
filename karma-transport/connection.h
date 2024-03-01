#pragma once
#include <cstddef>
#include <memory>
#include <optional>

#include "co_context/all.hpp"
#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "error.h"
#include "frame.h"
#include "write_task.h"
namespace transport {
class connection {
   public:
    connection(std::unique_ptr<co_context::socket> socket, std::string addr, uint8_t port);
    ~connection() = default;
    co_context::task<std::optional<connection_error>> write(frame& f);
    std::optional<std::unique_ptr<frame>> parse_frame();
    co_context::task<std::unique_ptr<frame>> read_frame();
    co_context::task<std::optional<connection_error>> write_frame(frame& f);

   private:
    co_context::task<void> loop();
    co_context::inet_address m_inet_address;
    std::unique_ptr<co_context::socket> m_socket;
    co_context::channel<write_task> m_write_task_chan;
    std::string m_buffer;
};
}  // namespace transport