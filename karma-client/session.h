#pragma once 
#include "co_context/io_context.hpp"
#include "karma-client/client_task.h"
#include "karma-client/header.h"
#include "karma-client/payload.h"
#include "karma-transport/connection.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#include "request.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
namespace client {
class session {
public:
    session(std::shared_ptr<transport::connection> connection)
        : m_connection(connection) {
        co_context::co_spawn(loop());
    }
    co_context::task<void> write(std::shared_ptr<client_task> task) {
        auto f = task->gen_frame();
        m_inflight_requests[f->m_seq] = task;
        auto result = co_await m_connection->write_frame(f);
        if (result.has_value()) {
            std::cout << "result: " << result.value() << std::endl;
        }
    };
private:
    co_context::task<void> loop() {
        while(1) {
            // auto f = co_await m_connection->read_frame();
            // if (f->is_response()) {
            //     std::cout << "response!!!!" << std::endl;
            //     auto seq = f->m_seq;
            //     auto task = m_inflight_requests[seq];
            //     auto s = std::dynamic_pointer_cast<ping_pong_task>(task);
            //     co_context::co_spawn(s->callback(f));
            // }
        }
    };
    std::shared_ptr<transport::connection> m_connection;
    std::unordered_map<uint32_t, std::shared_ptr<client_task>> m_inflight_requests;
};
}