#pragma once
// #include "karma-raft/raft.h"
#include <string>
#include <vector>

#include "karma-raft/raft.hh"
#include "karma-store/sivir.h"
namespace service {
class temp_persistence {
   public:
    temp_persistence() {}
    ~temp_persistence() {}
    co_context::task<> store_term_and_vote(raft::term_t term, raft::server_id vote) {
        m_term = term;
        m_server_id = vote;
        co_return;
    }
    co_context::task<std::pair<raft::term_t, raft::server_id>> load_term_and_vote() {
        std::pair<raft::term_t, raft::server_id> ret;
        ret.first = m_term;
        ret.second = m_server_id;
        co_return ret;
    }
    co_context::task<> store_commit_idx(raft::index_t idx) {
        m_commit_idx = idx;
        co_return;
    }
    co_context::task<raft::index_t> load_commit_idx() { co_return m_commit_idx; }

    co_context::task<> store_log_entries(const std::vector<raft::log_entry_ptr> &entries) {
        m_entries = entries;
        co_return;
    }
    co_context::task<raft::log_entries> load_log() { co_return m_entries; }
    co_context::task<raft::snapshot_descriptor> load_snapshot_descriptor() {
        raft::snapshot_descriptor snp;
        snp.id = 0;
        snp.idx = 0;
        snp.term = 0;
        raft::config_member member1(raft::server_address(1, "127.0.0.1:8888"), true);
        raft::config_member member2(raft::server_address(2, "127.0.0.1:8889"), true);
        raft::config_member member3(raft::server_address(3, "127.0.0.1:8890"), true);
        snp.config.current.insert(member1);
        snp.config.current.insert(member2);
        snp.config.current.insert(member3);
        co_return snp;
    }
    co_context::task<> truncate_log(raft::index_t idx) { co_return; }
    co_context::task<> abort() { co_return; }

   private:
    // const std::string SIVIR_TERM_KEY = "RAFT_TERM";
    // const std::string SIVIR_VOTE_KEY = "RAFT_VOTE";
    // just in the memory
    // std::shared_ptr<store::sivir> m_sivir;
    raft::index_t m_commit_idx;
    raft::term_t m_term;
    raft::server_id m_server_id;
    raft::index_t m_index;
    std::vector<raft::log_entry_ptr> m_entries;
};
}  // namespace service
