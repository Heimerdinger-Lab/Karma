#pragma once
#include "raft/raft.h"
class raft_rpc : public rpc {
public:
    // 发送rpc
    co_context::task<> send_append_entries(server_id id, const append_request& append_request) override;
    void send_append_entries_reply(server_id id, const append_reply& reply) override;
    void send_vote_request(server_id id, const vote_request& vote_request) override;
    void send_vote_reply(server_id id, const vote_reply& vote_reply) override;
    void send_timeout_now(server_id, const term_t& current_term) override;
    co_context::task<> abort() override;

    // 接受rpc消息，分发给m_server
    void append_entries(server_id from, append_request append_request);
    void append_entries_reply(server_id from, append_reply reply);
    void request_vote(server_id from, vote_request vote_request);
    void request_vote_reply(server_id from, vote_reply vote_reply);
    void timeout_now_request(server_id from, term_t current_term);
private:    
    

};