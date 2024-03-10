#pragma once
// #include "karma-raft/raft.h"
#include <memory>
#include <variant>

#include "karma-client/client.h"
#include "karma-raft/raft.hh"
namespace service {
class raft_rpc {
   public:
    raft_rpc(raft::server_id id, std::unique_ptr<client::client> client)
        : m_id(id), m_client(std::move(client)) {}
    ~raft_rpc() {}
    co_context::task<raft::snapshot_reply> send_snapshot(raft::server_id server_id,
                                                         const raft::install_snapshot& snap) {
        //
    }
    co_context::task<> send_append_entries(raft::server_id id,
                                           const raft::append_request& append_request) {
        // m_client->append_entry(raft::server_address start, raft::server_address
        // target, std::string msg)
        co_await m_client->append_entry(m_id, id, append_request);
    }
    co_context::task<> send_append_entries_reply(raft::server_id id,
                                                 const raft::append_reply& reply) {
        co_await m_client->append_entry_reply_(m_id, id, reply);
    }
    co_context::task<> send_vote_request(raft::server_id id,
                                         const raft::vote_request& vote_request) {
        co_await m_client->vote_request_(m_id, id, vote_request);
    }
    co_context::task<> send_vote_reply(raft::server_id id, const raft::vote_reply& vote_reply) {
        co_await m_client->vote_reply_(m_id, id, vote_reply);
    }
    co_context::task<> send_timeout_now(raft::server_id id, const raft::timeout_now& timeout_now) {
        co_await m_client->time_out(m_id, id, timeout_now);
    }
    co_context::task<> send_read_quorum(raft::server_id id, const raft::read_quorum& read_quorum) {
        co_await m_client->read_quorum(m_id, id, read_quorum);
    }
    co_context::task<> send_read_quorum_reply(raft::server_id id,
                                              const raft::read_quorum_reply& read_quorum_reply) {
        co_await m_client->read_quorum_reply_(m_id, id, read_quorum_reply);
    }
    co_context::task<raft::read_barrier_reply> forward_read_barrier(raft::server_id id) {
        // two-way rpc
        auto reply = co_await m_client->forward_read_barrier(m_id, id);
        if (reply.has_value()) {
            if (reply.value()->success()) {
                auto index = reply.value()->reply();
                co_return index;
            } else {
                co_return std::monostate{};
            }
        }
        co_return std::monostate{};
    }
    co_context::task<raft::add_entry_reply> forward_add_entry(raft::server_id id, std::string& key,
                                                              std::string& value) {
        // two-way rpc
        auto reply = co_await m_client->forward_add_entry(m_id, id, key, value);
        if (reply.has_value()) {
            if (reply.value()->success()) {
                auto entry_id = reply.value()->reply();
                co_return entry_id;
            } else {
                co_return raft::transient_error("", 0);
            }
        }
        co_return raft::transient_error("", 0);
    }
    co_context::task<> abort() {}

   private:
    std::unique_ptr<client::client> m_client;
    raft::server_id m_id;
};
}  // namespace service
