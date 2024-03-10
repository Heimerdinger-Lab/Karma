#pragma once
// #include "karma-raft/raft.h"
#include <string>
#include <vector>

#include "karma-raft/raft.hh"
#include "karma-store/sivir.h"
namespace service {
class raft_persistence {
   public:
    raft_persistence(std::shared_ptr<store::sivir> sivir) : m_sivir(sivir) {}
    ~raft_persistence() {}
    co_context::task<> store_term_and_vote(raft::term_t term, raft::server_id vote) {
        write_options opt;
        std::string key = SIVIR_TERM_KEY;
        std::string value = std::to_string(term);
        co_await m_sivir->put(opt, key, value);
        key = SIVIR_VOTE_KEY;
        value = std::to_string(vote);
        co_await m_sivir->put(opt, key, value);
    }
    co_context::task<std::pair<raft::term_t, raft::server_id>> load_term_and_vote() {
        std::pair<raft::term_t, raft::server_id> ret;
        std::string key = SIVIR_TERM_KEY;
        std::string value;
        co_await m_sivir->get(key, value);
        ret.first = std::stoi(value);
        value = "";
        key = SIVIR_VOTE_KEY;
        co_await m_sivir->get(key, value);
        ret.second = std::stoi(value);
        co_return ret;
    }
    co_context::task<> store_commit_idx(raft::index_t idx) {}
    co_context::task<raft::index_t> load_commit_idx() {}

    co_context::task<> store_log_entries(const std::vector<raft::log_entry_ptr> &entries) {
        co_return;
    }
    co_context::task<raft::log_entries> load_log() {}

    co_context::task<> truncate_log(raft::index_t idx) {}
    co_context::task<> abort() {}

   private:
    const std::string SIVIR_TERM_KEY = "RAFT_TERM";
    const std::string SIVIR_VOTE_KEY = "RAFT_VOTE";
    // just in the memory
    std::shared_ptr<store::sivir> m_sivir;
    raft::term_t m_term;
    raft::server_id m_server_id;
    raft::index_t m_index;
    std::vector<raft::log_entry_ptr> m_entries;
};
}  // namespace service
