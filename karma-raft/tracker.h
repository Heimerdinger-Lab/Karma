#pragma once 
#include "karma-raft/raft.h"
#include "karma-raft/common.h"
#include <unordered_map>
#include <unordered_set>
namespace raft {
class follower_progress {
public:
    const server_id m_server_id;
    index_t m_next_idx;
    index_t m_match_idx = index_t(0);
    index_t m_commit_idx = index_t(0);
    void accepted(index_t idx) {
        m_match_idx = std::max(idx, m_match_idx);
        m_next_idx = std::max(idx + index_t(1), m_next_idx);
    }
    follower_progress(server_id id, index_t next_idx) 
        : m_server_id(id)
        , m_next_idx(next_idx) {}
};
using progress = std::unordered_map<server_id, follower_progress>;

class tracker : private progress {
public:
    follower_progress* find(server_id id) {
        auto it = this->progress::find(id);
        return it == this->progress::end() ? nullptr : &it->second;
    }
    
};

class election_tracker {
    std::unordered_set<server_id> m_suffrage;
    std::unordered_set<server_id> m_responded;
    size_t m_granted = 0;
public: 
    election_tracker(const config_member_set& configuration) {
        for (const auto&s : configuration) {
            m_suffrage.emplace(s);
        }
    }
    bool register_vote(server_id from, bool granted) {
        if (m_suffrage.find(from) == m_suffrage.end()) {
            return false;
        }
        if (m_responded.emplace(from).second) {
            m_granted += granted;
        }
        return true;
    }
};
enum class vote_result {
    UNKNOW = 0,
    WON,
    LOST,
};
struct server_address {
    server_id id;
};
class votes {
    server_address_set m_voters;
    election_tracker m_current;
public:
    void register_vote(server_id from, bool granted);
    vote_result tally_votes() const;
    const server_address_set& voters() const {
        return m_voters;
    }
};
}
