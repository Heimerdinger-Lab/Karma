#pragma once 
#include "co_context/lazy_io.hpp"
#include "karma-client/tasks/echo_task.h"
#include "karma-client/tasks/append_entry_task.h"
#include "karma-client/tasks/vote_task.h"
#include "karma-client/tasks/time_out_task.h"
#include "karma-client/tasks/read_quorum_task.h"
#include "karma-client/tasks/read_task.h"
#include "karma-client/tasks/write_task.h"
#include "karma-raft/raft.hh"
#include "karma-client/session_manager.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <sys/socket.h>

namespace client {

class client {
public:
    client(std::map<uint64_t, std::string> members) 
        : m_session_manager (std::make_unique<session_manager>())
        {
        for (auto& item : members) {
            m_members[item.first] = parse_raft_server_address(item.second);
        }
    }
    co_context::task<void> rpc_timeout() {
        using namespace std::literals;
        co_await co_context::timeout(3s);   
    }
    co_context::task<std::shared_ptr<echo_reply>> echo(raft::server_id start, raft::server_id target, std::string msg) {
        std::shared_ptr<co_context::channel<std::shared_ptr<echo_reply>>> prom = std::make_shared<co_context::channel<std::shared_ptr<echo_reply>>>();
        std::shared_ptr<echo_request> req = std::make_shared<echo_request>(start, 0, msg, prom);
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
        
        co_await session.value()->request(req);
        auto reply = co_await prom->acquire();
        co_return reply;
    } 
    co_context::task<> append_entry(raft::server_id start, raft::server_id target, const raft::append_request& append_request) {
        std::shared_ptr<append_entry_request> req = std::make_shared<append_entry_request>(start, 0, append_request.current_term, append_request.prev_log_idx, append_request.prev_log_term, append_request.leader_commit_idx);
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
        if (session.has_value()) {
            co_await session.value()->request(req);
        }
    } 
    co_context::task<> append_entry_reply_(raft::server_id start, raft::server_id target, const raft::append_reply& reply) {
        std::shared_ptr<append_entry_reply> req = std::make_shared<append_entry_reply>(start, 0, reply.current_term, reply.commit_idx, reply.result);
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
        if (session.has_value()) {
            co_await session.value()->request(req);
        }
    } 
    co_context::task<> vote_request_(raft::server_id start, raft::server_id target, const raft::vote_request& vote_request_) {
        std::shared_ptr<vote_request> req = std::make_shared<vote_request>(start, 0, vote_request_.current_term, vote_request_.last_log_idx, vote_request_.last_log_term, vote_request_.is_prevote, vote_request_.force);
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
        if (session.has_value()) {
            co_await session.value()->request(req);
        }
    } 
    // co_context::task<> vote_reply_(raft::server_id start, raft::server_id target, const raft::vote_reply& vote_reply) {
    //     std::shared_ptr<vote_reply> req = std::make_shared<vote_reply>(start, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> time_out(raft::server_id start, raft::server_id target, const raft::timeout_now& timeout_now) {
    //     std::shared_ptr<time_out_request> req = std::make_shared<time_out_request>(start, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> read_quorum(raft::server_id start, raft::server_id target, const raft::read_quorum& read_quorum) {
    //     std::shared_ptr<read_quorum_request> req = std::make_shared<read_quorum_request>(start, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> read_quorum_reply_(raft::server_id start, raft::server_id target, const raft::read_quorum_reply& read_quorum_reply) {
    //     std::shared_ptr<read_quorum_reply> req = std::make_shared<read_quorum_reply>(start, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 

    // for outer
    co_context::task<std::shared_ptr<read_reply>> cli_read(raft::server_id group_id, raft::server_id target, std::string key) {
        std::shared_ptr<co_context::channel<std::shared_ptr<read_reply>>> prom = std::make_shared<co_context::channel<std::shared_ptr<read_reply>>>();
        std::shared_ptr<read_request> req = std::make_shared<read_request>(group_id, key, prom);
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
        co_await session.value()->request(req);
        auto reply = co_await prom->acquire();
        co_return reply;
    } 
    co_context::task<std::shared_ptr<write_reply>> cli_write(raft::group_id group_id, raft::server_id target, std::string key, std::string value) {
        std::shared_ptr<co_context::channel<std::shared_ptr<write_reply>>> prom = std::make_shared<co_context::channel<std::shared_ptr<write_reply>>>();
        std::shared_ptr<write_request> req = std::make_shared<write_request>(group_id, key, value, prom);
        auto session = co_await m_session_manager->get_composite_session(m_members[target].first, m_members[target].second);
        co_await session.value()->request(req);
        auto reply = co_await prom->acquire();
        co_return reply;
        
    } 
private:
    using address = std::pair<std::string, uint16_t>;
    address parse_raft_server_address(std::string addr) {
        
    };
    std::unique_ptr<session_manager> m_session_manager;
    std::map<uint64_t, address> m_members;
};
}