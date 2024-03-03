#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>

#include "co_context/io_context.hpp"
#include "karma-client/tasks/cli_echo_task.h"
#include "karma-client/tasks/cli_read_task.h"
#include "karma-client/tasks/cli_write_task.h"
#include "karma-client/tasks/task.h"
#include "karma-transport/connection.h"

namespace client {
class session {
   public:
    session(std::unique_ptr<transport::connection> connection)
        : m_connection(std::move(connection)) {
        co_context::co_spawn(loop());
    }
    co_context::task<bool> write(task& task_);
    bool valid() { return m_connection->valid(); }

   private:
    co_context::task<void> loop();
    std::unique_ptr<transport::connection> m_connection;
    std::map<uint32_t, task*> m_inflight_requests;
};
}  // namespace client