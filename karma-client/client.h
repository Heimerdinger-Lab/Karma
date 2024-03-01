#pragma once
#include <sys/socket.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "co_context/lazy_io.hpp"
#include "karma-client/session_manager.h"
#include "karma-client/tasks/append_entry_task.h"
#include "karma-client/tasks/echo_task.h"
#include "karma-client/tasks/read_quorum_task.h"
#include "karma-client/tasks/read_task.h"
#include "karma-client/tasks/time_out_task.h"
#include "karma-client/tasks/vote_task.h"
#include "karma-client/tasks/write_task.h"
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
    co_context::task<std::unique_ptr<echo_reply>> echo(raft::server_id start,
                                                       raft::server_id target, std::string msg);
    co_context::task<> append_entry(raft::server_id start, raft::server_id target,
                                    const raft::append_request& append_request);
    co_context::task<> append_entry_reply_(raft::server_id start, raft::server_id target,
                                           const raft::append_reply& reply);
    co_context::task<> vote_request_(raft::server_id start, raft::server_id target,
                                     const raft::vote_request& vote_request_);
    co_context::task<> vote_reply_(raft::server_id start, raft::server_id target,
                                   const raft::vote_reply& vote_reply_);
    co_context::task<> time_out(raft::server_id start, raft::server_id target,
                                const raft::timeout_now& timeout_now_);
    co_context::task<> read_quorum(raft::server_id start, raft::server_id target,
                                   const raft::read_quorum& read_quorum);
    co_context::task<> read_quorum_reply_(raft::server_id start, raft::server_id target,
                                          const raft::read_quorum_reply& read_quorum_reply_);

    co_context::task<std::unique_ptr<read_reply>> cli_read(raft::server_id group_id,
                                                           raft::server_id target, std::string key);
    co_context::task<std::unique_ptr<write_reply>> cli_write(raft::group_id group_id,
                                                             raft::server_id target,
                                                             std::string key, std::string value);

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