#pragma once
// #include "karma-raft/raft.h"
#include "karma-raft/raft.hh"
#include <vector>
namespace service {
class raft_persistence : public raft::persistence {
public:
    ~raft_persistence() {}
    co_context::task<> store_term_and_vote(raft::term_t term, raft::server_id vote) override {}
    co_context::task<std::pair<raft::term_t, raft::server_id>> load_term_and_vote() override {}
    co_context::task<> store_commit_idx(raft::index_t idx) override {}
    co_context::task<raft::index_t> load_commit_idx() override {}

    co_context::task<> store_log_entries(const std::vector<raft::log_entry_ptr>& entries) override {}
    // co_context::task<raft::log_entry_vec> load_log() override;
    
    co_context::task<> truncate_log(raft::index_t idx) override {}
    co_context::task<> abort() override {}
private:
    // just in the memory
    raft::term_t m_term;
    raft::server_id m_server_id;
    raft::index_t m_index;
    std::vector<raft::log_entry_ptr> m_entries;
};
}
