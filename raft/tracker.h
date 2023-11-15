#pragma once 
#include "raft.h"
#include "common.h"
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

};
enum class vote_result {
    UNKNOW = 0,
    WON,
    LOST,
};
struct server_address {
    server_id id;
};
struct server_address_hash {
    using is_transparent = void;
};
using server_address_set = std::unordered_set<server_address, server_address_hash, std::equal_to<>>;
class votes {
    server_address_set _voters;
public:
    void register_vote(server_id from, bool granted);
    vote_result tally_votes() const;
    const server_address_set& voters() const {
        return _voters;
    }

};
}
