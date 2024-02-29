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
    co_context::task<void> write(std::shared_ptr<task> task) {
        auto f = task->gen_frame();
        m_inflight_requests[f->m_seq] = task;
        auto result = co_await m_connection->write_frame(f);
        if (result.has_value()) {
            std::cout << "result: " << result.value() << std::endl;
        }
    };

   private:
    co_context::task<void> loop() {
        while (1) {
            auto f = co_await m_connection->read_frame();
            if (f->is_response()) {
                std::cout << "response!!!!" << std::endl;
                auto seq = f->m_seq;
                auto task = m_inflight_requests[seq];
                if (f->m_operation_code == karma_rpc::OperationCode_ECHO) {
                    auto s = std::dynamic_pointer_cast<echo_request>(task);
                    co_context::co_spawn(s->callback(f));
                } else if (f->m_operation_code == karma_rpc::OperationCode_READ_TASK) {
                    auto s = std::dynamic_pointer_cast<read_request>(task);
                    co_context::co_spawn(s->callback(f));
                } else if (f->m_operation_code == karma_rpc::OperationCode_WRITE_TASK) {
                    auto s = std::dynamic_pointer_cast<write_request>(task);
                    // co_context::co_spawn(s->callback(f));
                    co_await s->callback(f);
                }
            }
        }
    };
    std::unique_ptr<transport::connection> m_connection;
    std::unordered_map<uint32_t, std::shared_ptr<task>> m_inflight_requests;
};
}  // namespace client