#pragma once
#include "server.h"
#include "raft.h"
#include "fsm.h"

class server_impl : public rpc_server, public server {
public:

    // implement rpc_server
    void append_entries(server_id from, append_request append_request) override;
    void append_entries_reply(server_id from, append_reply reply) override;
    void request_vote(server_id from, vote_request vote_request) override;
    void request_vote_reply(server_id from, vote_reply vote_reply) override;
    void timeout_now_request(server_id from,  term_t current_term) override;

    // implement server
    co_context::task<> start() override;
    co_context::task<> abort() override;
    void tick() override;
    server_id id() const override;

private:
    co_context::task<> io_fiber(index_t stable_idx);
    co_context::task<> applier_fiber();
    template <typename Message> 
    void send_message(server_id id, Message m);

private:
    // m_rpc中有一个m_server，就是这个server_impl类
    // m_rpc就会将接收到的rpc分发给这个对象
    std::unique_ptr<rpc> m_rpc;
    std::unique_ptr<state_machine> m_state_machine;
    std::unique_ptr<persistence> m_persistence;
    std::unique_ptr<fsm> m_fsm;
    server_id m_server_id;
    index_t m_applied_idx;
    using applier_fiber_message = std::variant<log_entry_vec>;
    co_context::channel<applier_fiber_message, 10> m_apply_entries;
    co_context::condition_variable m_applied_index_changed;
    co_context::mutex m_applied_index_changed_mtx;
};
