#pragma once
#include "karma-raft/raft.h"
class raft_rpc : public raft::rpc {
public:
    // 发送rpc
    co_context::task<> send_append_entries(raft::server_id id, const raft::append_request& append_request) override;
    void send_append_entries_reply(raft::server_id id, const raft::append_reply& reply) override;
    void send_vote_request(raft::server_id id, const raft::vote_request& vote_request) override;
    void send_vote_reply(raft::server_id id, const raft::vote_reply& vote_reply) override;
    void send_timeout_now(raft::server_id, const raft::timeout_now& timeout_now) override;
    co_context::task<> abort() override;

    // 接受rpc消息，分发给m_server
    void append_entries(raft::server_id from, raft::append_request append_request);
    void append_entries_reply(raft::server_id from, raft::append_reply reply);
    void request_vote(raft::server_id from, raft::vote_request vote_request);
    void request_vote_reply(raft::server_id from, raft::vote_reply vote_reply);
    void timeout_now_request(raft::server_id from, raft::timeout_now timeout_now);
private:    
    

};