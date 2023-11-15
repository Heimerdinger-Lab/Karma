#pragma once 
#include <stdint.h>
#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <co_context/all.hpp>

#include "common.h"
#include "rpc_message.h"
namespace raft {
class state_machine {
public:
    virtual ~state_machine() {}
    
    virtual co_context::task<> apply(std::vector<command_cref> command) = 0;
    virtual co_context::task<> abort() = 0;
};

class rpc {
protected:
    rpc_server* m_server = nullptr;
public:
    virtual ~rpc() {}
    virtual co_context::task<> send_append_entries(server_id id, const append_request& append_request) = 0;
    virtual void send_append_entries_reply(server_id id, const append_reply& reply) = 0;
    virtual void send_vote_request(server_id id, const vote_request& vote_request) = 0;
    virtual void send_vote_reply(server_id id, const vote_reply& vote_reply) = 0;
    virtual void send_timeout_now(server_id, const term_t& current_term) = 0;
    virtual co_context::task<> abort() = 0;
private:
    friend rpc_server;
};

class rpc_server {
public:
    virtual ~rpc_server() {};
    virtual void append_entries(server_id from, append_request append_request) = 0;
    virtual void append_entries_reply(server_id from, append_reply reply) = 0;
    virtual void request_vote(server_id from, vote_request vote_request) = 0;
    virtual void request_vote_reply(server_id from, vote_reply vote_reply) = 0;
    virtual void timeout_now_request(server_id from, term_t current_term) = 0;
    void set_rpc_server(class rpc *rpc) { rpc->m_server = this; }
};


class persistence {
public:
    virtual ~persistence() {}
    virtual co_context::task<> store_term_and_vote(term_t term, server_id vote) = 0;
    virtual co_context::task<std::pair<term_t, server_id>> load_term_and_vote() = 0;
    virtual co_context::task<> store_commit_idx(index_t idx) = 0;
    virtual co_context::task<index_t> load_commit_idx() = 0;

    virtual co_context::task<> store_log_entries(const std::vector<log_entry_ptr>& entries) = 0;
    virtual co_context::task<log_entry_vec> load_log() = 0;
    // committed idx <= stable idx, 所以即使是stabled entries也有可能会被truncate的
    virtual co_context::task<> truncate_log(index_t idx) = 0;
    virtual co_context::task<> abort() = 0;
};
}
