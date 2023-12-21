#pragma once 
#include "karma-raft/raft.h"
#include "karma-raft/common.h"
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
namespace raft {
class follower_progress {
public:
    const server_id m_server_id;
    index_t m_next_idx;
    index_t m_match_idx = index_t(0);
    index_t m_commit_idx = index_t(0);
    read_id m_max_acked_read = read_id(0);
    bool m_can_vote = true;
    enum class state {
        // In this state only one append entry is send until matching index is found
        PROBE,
        // In this state multiple append entries are sent optimistically
        PIPELINE,
        // In this state snapshot has been transferred
        SNAPSHOT
    };
    state state = state::PROBE;
    bool probe_sent = false;
    size_t in_flight = 0;
    static constexpr size_t max_in_flight = 10;
    bool is_stray_reject(const append_reply::rejected& r);

    void accepted(index_t idx) {
        m_match_idx = std::max(idx, m_match_idx);
        m_next_idx = std::max(idx + index_t(1), m_next_idx);
    }
    void become_probe();
    void become_pipeline();
    void become_snapshot(index_t snp_idx);

    follower_progress(server_id id, index_t next_idx) 
        : m_server_id(id)
        , m_next_idx(next_idx) {}

    bool can_send_to();
};
using progress = std::unordered_map<server_id, follower_progress>;

class tracker : private progress {
    std::unordered_set<server_id> m_current_voters;
    std::unordered_set<server_id> m_previous_voters;

public:    
    using progress::begin, progress::end, progress::cbegin, progress::cend, progress::size;

    follower_progress* find(server_id id) {
        auto it = this->progress::find(id);
        return it == this->progress::end() ? nullptr : &it->second;
    }
    void set_configuration(const configuration& configuration, index_t next_idx);
    template<typename Index> Index committed(Index prev_commit_idx);
    class activity_tracker {
        tracker& _tracker;
        size_t _cur = 0;
        size_t _prev = 0;
        activity_tracker(tracker& t) : _tracker(t) {}
    public:
        void operator()(server_id id) {
            _cur += _tracker.m_current_voters.contains(id);
            _prev += _tracker.m_previous_voters.contains(id);
        }

        operator bool() const {
            bool active = _cur >= _tracker.m_current_voters.size()/2 + 1;
            if (!_tracker.m_previous_voters.empty()) {
                active &= _prev >= _tracker.m_previous_voters.size()/2 + 1;
            }
            return active;
        }
        friend tracker;
    };

    activity_tracker get_activity_tracker() {
        return activity_tracker(*this);
    }

    friend activity_tracker;
};



// vote 家族
enum class vote_result {
    UNKNOWN = 0,
    WON,
    LOST,
};

std::ostream& operator<<(std::ostream& os, const vote_result& v);
class election_tracker {
    std::unordered_set<server_id> m_suffrage;
    std::unordered_set<server_id> m_responded;
    size_t m_granted = 0;
public: 
    election_tracker(const config_member_set& configuration) {
        for (const auto&s : configuration) {
            if (s.can_vote) {
                m_suffrage.emplace(s.addr.id);
            }
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
    vote_result tally_votes() const {
        auto quorum = m_suffrage.size() / 2 + 1;
        if (m_granted >= quorum) {
            return vote_result::WON;
        }
        auto unknown = m_suffrage.size() - m_responded.size();
        return m_granted + unknown >= quorum ? vote_result::UNKNOWN : vote_result::LOST;
    }
};

// 通过配置文件指定

class votes {
    server_address_set m_voters;
    election_tracker m_current;
    std::optional<election_tracker> m_previous;
public:    
    votes(configuration configuration);
    void register_vote(server_id from, bool granted);
    vote_result tally_votes() const;
    const server_address_set& voters() const {
        return m_voters;
    }
};


}