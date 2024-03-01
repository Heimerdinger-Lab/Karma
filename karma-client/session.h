#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "co_context/io_context.hpp"
#include "karma-client/tasks/echo_task.h"
#include "karma-client/tasks/read_task.h"
#include "karma-client/tasks/task.h"
#include "karma-client/tasks/write_task.h"
#include "karma-transport/connection.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
namespace client {
class session {
   public:
    session(std::unique_ptr<transport::connection> connection)
        : m_connection(std::move(connection)) {
        co_context::co_spawn(loop());
    }
    co_context::task<void> write(task& task_);

   private:
    co_context::task<void> loop();
    std::unique_ptr<transport::connection> m_connection;
    std::unordered_map<uint32_t, task*> m_inflight_requests;
};
}  // namespace client