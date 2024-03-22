#pragma once
#include <sys/socket.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "co_context/lazy_io.hpp"
#include "karma-client/session_manager.h"
#include "karma-client/tasks/append_entry_task.h"
#include "karma-client/tasks/cli_echo_task.h"
#include "karma-client/tasks/cli_read_task.h"
#include "karma-client/tasks/cli_write_task.h"
#include "karma-client/tasks/forward_cli_write_task.h"
#include "karma-client/tasks/forward_read_barrier_task.h"
#include "karma-client/tasks/read_quorum_task.h"
#include "karma-client/tasks/time_out_task.h"
#include "karma-client/tasks/vote_task.h"
#include "karma-raft/raft.hh"

namespace client {

class client {
   public:
    client(std::map<uint64_t, std::string> members)
        : m_session_manager(std::make_unique<session_manager>()) {
        for (auto& item : members) {
            m_members[item.first] = parse_raft_server_address(item.second);
        }
    }
    co_context::task<void> rpc_timeout() {
        using namespace std::literals;
        co_await co_context::timeout(3s);
    }
    // one way rpc
    co_context::task<bool> append_entry(raft::server_id start, raft::server_id target,
                                        const raft::append_request& append_request);
    co_context::task<bool> append_entry_reply_(raft::server_id start, raft::server_id target,
                                               const raft::append_reply& reply);
    co_context::task<bool> vote_request_(raft::server_id start, raft::server_id target,
                                         const raft::vote_request& vote_request_);
    co_context::task<bool> vote_reply_(raft::server_id start, raft::server_id target,
                                       const raft::vote_reply& vote_reply_);
    co_context::task<bool> time_out(raft::server_id start, raft::server_id target,
                                    const raft::timeout_now& timeout_now_);
    co_context::task<bool> read_quorum(raft::server_id start, raft::server_id target,
                                       const raft::read_quorum& read_quorum);
    co_context::task<bool> read_quorum_reply_(raft::server_id start, raft::server_id target,
                                              const raft::read_quorum_reply& read_quorum_reply_);

    // two way rpc
    co_context::task<std::optional<std::unique_ptr<forward_cli_write_task_reply>>>
    forward_cli_write(raft::server_id start, raft::server_id target, std::string& key,
                      std::string& value) {
        auto prom =
            std::make_unique<co_context::channel<std::unique_ptr<forward_cli_write_task_reply>>>();
        auto req = std::make_unique<forward_cli_write_task>(start, target, key, value);
        req->set_prom(prom.get());
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                         m_members[target].second);
        if (session.has_value()) {
            co_await session.value().get().request(*req);
            auto reply = co_await prom->acquire();
            co_return reply;
        }
        co_return std::nullopt;
    }

    co_context::task<std::optional<std::unique_ptr<forward_read_barrier_task_reply>>>
    forward_read_barrier(raft::server_id start, raft::server_id target) {
        auto prom = std::make_unique<
            co_context::channel<std::unique_ptr<forward_read_barrier_task_reply>>>();
        auto req = std::make_unique<forward_read_barrier_task>(start, target);
        req->set_prom(prom.get());
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                         m_members[target].second);
        if (session.has_value()) {
            co_await session.value().get().request(*req);
            auto reply = co_await prom->acquire();
            co_return reply;
        }
        co_return std::nullopt;
    }

    // two way rpc
    co_context::task<std::optional<std::unique_ptr<cli_echo_reply>>> cli_echo(
        raft::server_id start, raft::server_id target, std::string msg);

    co_context::task<std::optional<std::unique_ptr<cli_read_reply>>> cli_read(
        raft::server_id group_id, raft::server_id target, std::string key);
    co_context::task<std::optional<std::unique_ptr<cli_write_reply>>> cli_write(
        raft::group_id group_id, raft::server_id target, std::string key, std::string value);

   private:
    using address = std::pair<std::string, uint16_t>;
    address parse_raft_server_address(std::string addr) {
        size_t pos = addr.find(':');
        // 提取IP地址和端口号
        std::string ip = addr.substr(0, pos);
        std::string port = addr.substr(pos + 1);
        return std::make_pair(ip, std::stoi(port));
    };
    std::unique_ptr<session_manager> m_session_manager;
    std::map<uint64_t, address> m_members;
};
}  // namespace client