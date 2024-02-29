#pragma once
// #include "karma-raft/raft.h"
#include "karma-client/client.h"
#include "karma-raft/raft.hh"
namespace service {
class raft_rpc : public raft::rpc {
   public:
    raft_rpc(raft::server_id id, std::shared_ptr<client::client> client)
        : m_id(id), m_client(client) {}
    ~raft_rpc() {}
    co_context::task<raft::snapshot_reply> send_snapshot(
        raft::server_id server_id, const raft::install_snapshot& snap) override {
        //
    }
    co_context::task<> send_append_entries(raft::server_id id,
                                           const raft::append_request& append_request) override {
        // m_client->append_entry(raft::server_address start, raft::server_address
        // target, std::string msg)
        co_await m_client->append_entry(m_id, id, append_request);
    }
    co_context::task<> send_append_entries_reply(raft::server_id id,
                                                 const raft::append_reply& reply) override {
        co_await m_client->append_entry_reply_(m_id, id, reply);
    }
    co_context::task<> send_vote_request(raft::server_id id,
                                         const raft::vote_request& vote_request) override {
        co_await m_client->vote_request_(m_id, id, vote_request);
    }
    co_context::task<> send_vote_reply(raft::server_id id,
                                       const raft::vote_reply& vote_reply) override {
        co_await m_client->vote_reply_(m_id, id, vote_reply);
    }
    co_context::task<> send_timeout_now(raft::server_id id,
                                        const raft::timeout_now& timeout_now) override {
        co_await m_client->time_out(m_id, id, timeout_now);
    }
    co_context::task<> send_read_quorum(raft::server_id id,
                                        const raft::read_quorum& read_quorum) override {
        co_await m_client->read_quorum(m_id, id, read_quorum);
    }
    co_context::task<> send_read_quorum_reply(
        raft::server_id id, const raft::read_quorum_reply& read_quorum_reply) override {
        co_await m_client->read_quorum_reply_(m_id, id, read_quorum_reply);
    }
    co_context::task<raft::read_barrier_reply> execute_read_barrier_on_leader(
        raft::server_id id) override {}
    co_context::task<raft::add_entry_reply> send_add_entry(raft::server_id id,
                                                           const raft::command& cmd) override {}
    co_context::task<raft::add_entry_reply> send_modify_config(
        raft::server_id id, const std::vector<raft::config_member>& add,
        const std::vector<raft::server_id>& del) override {}
    void on_configuration_change(raft::server_address_set add,
                                 raft::server_address_set del) override {}
    co_context::task<> abort() override {}

   private:
    std::shared_ptr<client::client> m_client;
    raft::server_id m_id;
};
}  // namespace service
