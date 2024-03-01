#pragma once
#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "co_context/task.hpp"
#include "karma-client/tasks/append_entry_task.h"
#include "karma-client/tasks/echo_task.h"
#include "karma-client/tasks/read_quorum_task.h"
#include "karma-client/tasks/read_task.h"
#include "karma-client/tasks/vote_task.h"
#include "karma-client/tasks/write_task.h"
// #include "karma-service/handler/handler.h"
#include <memory>

#include "karma-service/raft_server.h"
#include "karma-transport/connection.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
namespace service {
class session {
   public:
    session(int sockfd, std::string addr, uint8_t port, raft_server& raft_server)
        : m_inet_address(addr, port), m_raft(raft_server) {
        m_conn =
            std::make_unique<transport::connection>(std::make_unique<co_context::socket>(sockfd),
                                                    m_inet_address.to_ip(), m_inet_address.port());
    }
    void process() { co_context::co_spawn(process0()); }
    co_context::task<> process0() {
        auto channel =
            std::make_unique<co_context::channel<std::unique_ptr<transport::frame>, 1024>>();
        co_context::co_spawn(read_loop(*channel, *m_conn));
        co_context::co_spawn(write_loop(*channel, *m_conn));
        co_return;
    }
    bool valid() {
        // session是否还存活
        // client和server都需要去检查
        // connection的析构函数中close socket
        // 而整个链接资源的释放是在session被移除时

        // read和write有一个loop失败，就置该session出错
        // read loop时read 出错，则退出循环
        // write loop时出错，退出循环
        return true;
    }
    co_context::task<void> read_loop(
        co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
        transport::connection& conn);
    co_context::task<void> write_loop(
        co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
        transport::connection& conn);

   private:
    std::unique_ptr<transport::connection> m_conn;
    co_context::inet_address m_inet_address;
    // std::unique_ptr<co_context::socket> m_socket;
    raft_server& m_raft;
};
};  // namespace service