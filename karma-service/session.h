#pragma once
#include <memory>

#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "co_context/task.hpp"
#include "karma-client/tasks/append_entry_task.h"
#include "karma-client/tasks/cli_echo_task.h"
#include "karma-client/tasks/cli_read_task.h"
#include "karma-client/tasks/cli_write_task.h"
#include "karma-client/tasks/read_quorum_task.h"
#include "karma-client/tasks/vote_task.h"
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
        m_channel =
            std::make_unique<co_context::channel<std::unique_ptr<transport::frame>, 1024>>();
    }
    void process() { co_context::co_spawn(process0()); }
    co_context::task<> process0() {
        co_context::co_spawn(read_loop(*m_channel, *m_conn));
        co_context::co_spawn(write_loop(*m_channel, *m_conn));
        co_return;
    }
    bool valid() { return m_conn->valid(); }
    co_context::task<void> read_loop(
        co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
        transport::connection& conn);
    co_context::task<void> write_loop(
        co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
        transport::connection& conn);

   private:
    std::unique_ptr<transport::connection> m_conn;
    std::unique_ptr<co_context::channel<std::unique_ptr<transport::frame>, 1024>> m_channel;
    co_context::inet_address m_inet_address;
    raft_server& m_raft;
};
};  // namespace service