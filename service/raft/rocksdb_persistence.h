#pragma once
#include "raft/raft.h"
class rocksdb_persistence : persistence {
public:
    co_context::task<> store_term_and_vote(term_t term, server_id vote) override;
    co_context::task<std::pair<term_t, server_id>> load_term_and_vote() override;
    co_context::task<> store_commit_idx(index_t idx) override;
    co_context::task<index_t> load_commit_idx() override;

    co_context::task<> store_log_entries(const std::vector<log_entry_ptr>& entries) override;
    co_context::task<log_entry_vec> load_log() override;
    
    co_context::task<> truncate_log(index_t idx) override;
    co_context::task<> abort() override;
};