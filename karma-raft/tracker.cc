#include "tracker.h"

raft::votes::votes(configuration configuration)
    : m_current(configuration.m_current)
{
    for (auto& item: configuration.m_current) {
        if (item.can_vote) {
            m_voters.insert(item.addr);
        }
    }; 
    for (auto& item: configuration.m_previous) {
        if (item.can_vote) {
            m_voters.insert(item.addr);
        }
    }; 
    if (configuration.is_joint()) {
        m_previous.emplace(configuration.m_previous);
    }
}

void raft::votes::register_vote(server_id from, bool granted) {
    bool registered = false;

    if (m_current.register_vote(from, granted)) {
        registered = true;
    }
    if (m_previous && m_previous->register_vote(from, granted)) {
        registered = true;
    }
    // if (!registered) {
    //     logger.info("Got a vote from unregistered server {} during election", from);
    // }
}

raft::vote_result raft::votes::tally_votes() const {
    if (m_previous) {
        // join 状态，won是需要双方都won！
        // 简称 win win 定理！
        auto previous_result = m_previous->tally_votes();
        if (previous_result != vote_result::WON) {
            return previous_result;
        }
    }
    return m_current.tally_votes();
}

std::ostream& raft::operator<<(std::ostream& os, const vote_result& v) {
    static const char *n;
    switch (v) {
    case vote_result::UNKNOWN:
        n = "UNKNOWN";
        break;
    case vote_result::WON:
        n = "WON";
        break;
    case vote_result::LOST:
        n = "LOST";
        break;
    }
    os << n;
    return os;
}

void raft::tracker::set_configuration(const configuration& configuration, index_t next_idx) {
    m_current_voters.clear();
    m_previous_voters.clear();

    // Swap out the current progress and then re-add
    // only those entries which are still present.
    progress old_progress = std::move(*this);

    auto emplace_simple_config = [&](const config_member_set& config, std::unordered_set<server_id>& voter_ids) {
        for (const auto& s : config) {
            if (s.can_vote) {
                voter_ids.emplace(s.addr.id);
            }
            auto newp = this->progress::find(s.addr.id);
            if (newp != this->progress::end()) {
                // Processing joint configuration and already added
                // an entry for this id.
                continue;
            }
            auto oldp = old_progress.find(s.addr.id);
            if (oldp != old_progress.end()) {
                newp = this->progress::emplace(s.addr.id, std::move(oldp->second)).first;
            } else {
                newp = this->progress::emplace(s.addr.id, follower_progress{s.addr.id, next_idx}).first;
            }
            newp->second.m_can_vote = s.can_vote;
        }
    };
    emplace_simple_config(configuration.m_current, m_current_voters);
    if (configuration.is_joint()) {
        emplace_simple_config(configuration.m_previous, m_previous_voters);
    }
}

// A sorted array of node match indexes used to find
// the pivot which serves as commit index of the group.
template<typename Index>
class match_vector {
    std::vector<Index> _match;
    // How many elements in the match array have a match index
    // larger than the previous commit index.
    size_t _count = 0;
    Index _prev_commit_idx;
public:
    explicit match_vector(Index prev_commit_idx, size_t reserve_size)
            : _prev_commit_idx(prev_commit_idx) {
        _match.reserve(reserve_size);
    }

    void push_back(Index match_idx) {
        if (match_idx > _prev_commit_idx) {
            _count++;
        }
        _match.push_back(match_idx);
    }
    bool committed() const {
        // 大于曾经的commit idx的数目超过半数，说明可以移动commit idx了
        return _count >= _match.size()/2 + 1;
    }
    Index commit_idx() {
        // logger.trace("{}: check committed count {} cluster size {}", std::is_same_v<Index, index_t> ? "commit" : "read barrier", _count, _match.size());
        // The index of the pivot node is selected so that all nodes
        // with a larger match index plus the pivot form a majority,
        // for example:
        // cluster size  pivot node     majority
        // 1             0              1
        // 2             0              2
        // 3             1              2
        // 4             1              3
        // 5             2              3
        //
        auto pivot = (_match.size() - 1) / 2;
        std::nth_element(_match.begin(), _match.begin() + pivot, _match.end());
        return _match[pivot];
    }
};

template<typename Index>
Index raft::tracker::committed(Index prev_commit_idx) {
    
    // 这里也需要考虑前后的配置不同
    auto push_idx = [] (match_vector<Index>& v, const follower_progress& p) {
        if constexpr (std::is_same_v<Index, index_t>) {
            v.push_back(p.m_match_idx);
        } else {
            v.push_back(p.m_max_acked_read);
        }
    };
    match_vector<Index> current(prev_commit_idx, m_current_voters.size());

    if (!m_previous_voters.empty()) {
        match_vector<Index> previous(prev_commit_idx, m_previous_voters.size());

        for (const auto& [id, p] : *this) {
            if (m_current_voters.contains(p.m_server_id)) {
                push_idx(current, p);
            }
            if (m_previous_voters.contains(p.m_server_id)) {
                push_idx(previous, p);
            }
        }
        if (!current.committed() || !previous.committed()) {
            return prev_commit_idx;
        }
        return std::min(current.commit_idx(), previous.commit_idx());
    } else {
        for (const auto& [id, p] : *this) {
            if (m_current_voters.contains(p.m_server_id)) {
                push_idx(current, p);
            }
        }
        if (!current.committed()) {
            return prev_commit_idx;
        }
        return current.commit_idx();
    }
}
bool raft::follower_progress::is_stray_reject(const append_reply::rejected& rejected) {
    // By precondition, we are the leader and `rejected.current_term` is equal to our term.
    // By definition of `match_idx` we know that at some point all entries up to and including
    // `match_idx` were the same in our log and the follower's log; ...
    if (rejected.non_matching_idx <= m_match_idx) {
        // ... in particular, entry `rejected.non_matching_idx` (which is <= `match_idx`) at some point
        // was the same in our log and the follower's log, but `rejected` claims they are different.
        // A follower cannot change an entry unless it enters a different term, but `rejected.current_term`
        // is equal to our term. Thus `rejected` must be stray.
        return true;
    }
    if (rejected.last_idx < m_match_idx) {
        // ... in particular, at some point the follower had to have an entry with index `rejected.last_idx + 1`
        // (because `rejected.last_idx < match_idx implies rejected.last_idx + 1 <= match_idx)
        // but `rejected` claims it doesn't have such entry now.
        // A follower cannot truncate a suffix of its log unless it enters a different term,
        // but `rejected.current_term` is equal to our term. Thus `rejected` must be stray.
        return true;
    }

    switch (state) {
    case follower_progress::state::PIPELINE:
        break;
    case follower_progress::state::PROBE:
        // In PROBE state we send a single append request `req` with `req.prev_log_idx == next_idx - 1`.
        // When the follower generates a rejected response `r`, it sets `r.non_matching_idx = req.prev_log_idx`.
        // Thus the reject either satisfies `rejected.non_matching_idx == next_idx - 1` or is stray.
        if (rejected.non_matching_idx != index_t(m_next_idx - 1)) {
            return true;
        }
        break;
    case follower_progress::state::SNAPSHOT:
        // any reject during snapshot transfer is stray one
        return true;
    default:
        assert(false);
    }
    return false;
}
bool raft::follower_progress::can_send_to() {
    switch (state) {
    case state::PROBE:
        return !probe_sent;
    case state::PIPELINE:
        // allow `max_in_flight` outstanding indexes
        // FIXME: make it smarter
        return in_flight < follower_progress::max_in_flight;
    case state::SNAPSHOT:
        // In this state we are waiting
        // for a snapshot to be transferred
        // before starting to sync the log.
        return false;
    }
    assert(false);
    return false;
}
template raft::index_t raft::tracker::committed(raft::index_t);
// template raft::read_id raft::tracker::committed(raft::read_id);
