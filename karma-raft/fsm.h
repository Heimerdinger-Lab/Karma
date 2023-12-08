#pragma once 
#include <optional>
#include <vector>

#include "co_context/all.hpp"
#include "karma-raft/raft.h"
#include "karma-raft/rpc_message.h"
#include "karma-raft/common.h"
#include "karma-raft/log.h"
#include "karma-raft/logical_clock.h"
#include "karma-raft/tracker.h"
namespace raft {
struct fsm_output {
    std::optional<std::pair<term_t, server_id>> term_and_vote;
    log_entry_vec log_entries;
    std::vector<std::pair<server_id, rpc_message>> messages;

    // entries to apply
    log_entry_vec committed_entries;
    bool state_changed = false;
    bool empty() const {

    };
};
class fsm;
struct follower {
    server_id current_leader_id;
};

struct candidate {
    // raft::votes votes;
    votes votes_;
};
class fsm;
struct leader {
    // tracker
    tracker m_tracker;
    const fsm &m_fsm;
public:
    leader(const class fsm& fsm_) : m_fsm(fsm_) {}
};


class fsm {
    server_id m_id;
    std::variant<follower, candidate, leader> m_state;
    term_t m_current_term;
    server_id m_voted_for;
    index_t m_commit_idx;
    log m_log;
    struct last_observed_state {
        term_t m_current_term;
        server_id m_vote_for;
        index_t m_commit_idx;
        term_t m_last_term;

        bool is_equal(const fsm& fsm) const {
            return m_current_term == fsm.m_current_term && m_vote_for == fsm.m_voted_for && m_commit_idx == fsm.m_commit_idx && m_last_term == fsm.m_log.last_term();
        }
        void advance(const fsm& fsm) {
            m_current_term =fsm.m_current_term;
            m_vote_for = fsm.m_voted_for;
            m_commit_idx = fsm.m_commit_idx;
            m_last_term = fsm.m_log.last_term();
        }
    } m_observed;
    fsm_output m_output;
    logical_clock m_clock;
    logical_clock::time_point m_last_election_time = logical_clock::min();

    // logical_clock::duration m_randomized_election_timeout = ELECTION_TIMEOUT + logical_clock::duration{1};
private:
    std::vector<std::pair<server_id, rpc_message>> m_messages;
    co_context::mutex m_events_mtx;
    co_context::condition_variable m_events;
    void maybe_commit();
    bool is_past_election_timeout() const;

    template<typename Message>
    void send_to(server_id to, Message&& m) {
        m_messages.push_back(std::make_pair(to, std::move(m)));
        m_events.notify_all();
    }
    void update_current_term(term_t current_term);

    void become_leader();
    void become_candidate();
    void become_follower(server_id leader);
    void replicate_to(bool allow_empty);
    void replicate();
    void append_entries(server_id from, append_request&& append_request);

    void append_entries_reply(server_id from, append_reply&& reply);
    void request_vote(server_id from, vote_request&& vote_request);
    void request_vote_reply(server_id from, vote_reply&& vote_reply);

    void advance_commit_idx(index_t leader_commit_idx);
    void advance_stable_idx(index_t idx);
    void tick_leader();
    void reset_election_timeout();
    candidate& candidate_state() {
        return std::get<candidate>(m_state);
    }

    const candidate& candidate_state() const {
        return std::get<candidate>(m_state);
    }

    follower& follower_state() {
        return std::get<follower>(m_state);
    }

    const follower& follower_state() const {
        return std::get<follower>(m_state);
    }

    void send_timeout_now(server_id);
public:
    explicit fsm(server_id id, term_t current_term, server_id voted_for, log log,
            index_t commit_idx);

    explicit fsm(server_id id, term_t current_term, server_id voted_for, log log);

    bool is_leader() const {
        return std::holds_alternative<leader>(m_state);
    }
    bool is_follower() const {
        return std::holds_alternative<follower>(m_state);
    }
    bool is_candidate() const {
        return std::holds_alternative<candidate>(m_state);
    }
    leader& leader_state() {
        return std::get<leader>(m_state);
    }
    const leader& leader_state() const {
        return std::get<leader>(m_state);
    }
    index_t log_last_idx() const {
        return m_log.last_idx();
    }
    term_t log_last_term() const {
        return m_log.last_term();
    }
    index_t commit_idx() const {
        return m_commit_idx;
    }
    std::optional<term_t> log_term_for(index_t idx) const {
        return m_log.term_for(idx);
    }
    server_id current_leader() const {
        if (is_leader()) {
            return m_id;
        } else if (is_candidate()) {
            return {};
        } else {
            return follower_state().current_leader_id;
        }
    }
    // Add an entry to in-memory log. The entry has to be
    // committed to the persistent Raft log afterwards.
    template<typename T> const log_entry&   add_entry(T command);

    co_context::task<fsm_output> poll_output();
    fsm_output get_output();

    void tick();
    template <typename Message>
    void step(server_id from, Message&& msg);

    template <typename Message>
    void step(server_id from, const leader& s, Message&& msg);
    template <typename Message>
    void step(server_id from, const candidate& s, Message&& msg);
    template <typename Message>
    void step(server_id from, const follower& s, Message&& msg);
    void stop();

    term_t get_current_term() const {
        return m_current_term;
    }

    // How much time has passed since last election or last
    // time we heard from a valid leader.
    logical_clock::duration election_elapsed() const {
        return m_clock.now() - m_last_election_time;
    }

    server_id id() const { return m_id; }

    
};
}
