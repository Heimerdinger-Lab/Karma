#pragma once 
#include "co_context/all.hpp"
#include "co_context/co/channel.hpp"
#include "co_context/co/when_any.hpp"
#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "karma-client/client_task.h"
#include "karma-client/error.h"
// #include "karma-raft/common.h"
#include "scylladb-raft/raft.hh"
#include "karma-client/session_manager.h"
// #include "karma-raft/rpc_message.h"
#include <memory>
#include <optional>
#include <sys/socket.h>
#include <variant>

namespace client {

// class client {
// public:
//     client() 
//         : m_session_manager (std::make_unique<session_manager>()){
//         co_context::co_spawn(loop());
//     }
//     co_context::task<void> rpc_timeout() {
//         using namespace std::literals;
//         co_await co_context::timeout(3s);   
//         // co_return std::monostate();
//     }
//     co_context::task<std::variant<raft::ping_pong_reply, client_error>> ping_pong(raft::server_address start, raft::server_address target, const raft::ping_pong_request& req) {
//         auto observer = std::make_shared<co_context::channel<std::variant<raft::ping_pong_reply, client_error>, 0>>();
//         std::shared_ptr<client_task> task = std::make_shared<ping_pong_task>(start, target, std::make_shared<raft::ping_pong_request>(req), observer);
//         co_await m_client_task_chan.release(task);
//         using namespace std::literals;
//         auto [idx, var] = co_await co_context::any(observer->acquire(), rpc_timeout());
//         if (std::holds_alternative<std::variant<raft::ping_pong_reply, client_error>>(var)) {
//             std::cout << "client got it" << std::endl;
//             auto var2 = std::get<std::variant<raft::ping_pong_reply, client_error>>(var);
//             co_return var2;
//         } else {
//             // timeout
//             co_return client_error::rpc_timeout;
//         }
//     } 
// private:
//     co_context::task<void> loop() {
//         while(1) {
//             auto task = co_await m_client_task_chan.acquire();
//             co_context::co_spawn(send_to_one(task));
//         };
//     }
//     co_context::task<void> send_to_one(std::shared_ptr<client_task> task) {
//         auto cs = co_await m_session_manager->get_composite_session(task->get_target_address().host, task->get_target_address().port);
//         if (cs == std::nullopt) {
//             // task->callback();
//         }
//         co_await cs.value()->request_to_one(task);
//     }
//     co_context::channel<std::shared_ptr<client_task>> m_client_task_chan;
//     std::unique_ptr<session_manager> m_session_manager;
// };
}