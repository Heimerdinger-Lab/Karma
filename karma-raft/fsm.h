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
    struct applied_snapshot {
        snapshot_descriptor snp;
        bool is_local;
    };
    std::optional<std::pair<term_t, server_id>> term_and_vote;
    log_entry_vec log_entries;
    std::vector<std::pair<server_id, rpc_message>> messages;

    // entries to apply
    log_entry_vec committed_entries;
    std::optional<applied_snapshot> snp;
    std::vector<snapshot_id> snps_to_drop;
    std::optional<config_member_set> configuration;
    std::optional<read_id> max_read_id_with_quorum;
    bool state_changed = false;
    bool abort_leadership_transfer;
    bool empty() const {
        return !term_and_vote &&
            log_entries.size() == 0 && messages.size() == 0 &&
            committed_entries.size() == 0 && !snp && snps_to_drop.empty() &&
            !configuration && !max_read_id_with_quorum;
    };
};
class fsm;
struct follower {
    server_id current_leader_id;
};

struct candidate {
    // raft::votes votes;
    votes votes_;
    bool is_prevote;
    candidate(configuration configuration, bool prevote) :
               votes_(std::move(configuration)), is_prevote(prevote) {}
};
class fsm;
struct leader {
    // tracker
    tracker m_tracker;
    const fsm &m_fsm;
    // 暂时不设置
    // std::unique_ptr<seastar::semaphore> log_limiter_semaphore;
    std::optional<logical_clock::time_point> m_stepdown;
    std::optional<server_id> timeout_now_sent;
    read_id last_read_id{0};
    bool last_read_id_changed = false;
    read_id max_read_id_with_quorum{0};
public:
    leader(const class fsm& fsm_) : m_fsm(fsm_) {}
    leader(leader&&) = default;
    ~leader();
};
struct fsm_config {
    // max size of appended entries in bytes
    size_t append_request_threshold;
    // Limit in bytes on the size of in-memory part of the log after
    // which requests are stopped to be admitted until the log
    // is shrunk back by a snapshot. Should be greater than
    // the sum of sizes of trailing log entries, otherwise the state
    // machine will deadlock.
    size_t max_log_size;
    // If set to true will enable prevoting stage during election
    bool enable_prevoting;
};

class fsm {
    server_id m_id;
    std::variant<follower, candidate, leader> m_state;
    term_t m_current_term;
    server_id m_voted_for;
    index_t m_commit_idx;
    log m_log;
    failure_detector& m_failure_detector;
    fsm_config m_config;
    bool m_abort_leadership_transfer = false;
    bool m_ping_leader = false;
    struct last_observed_state {
        term_t m_current_term;
        server_id m_voted_for;
        index_t m_commit_idx;
        index_t m_last_conf_idx;
        term_t m_last_term;
        bool m_abort_leadership_transfer;

        bool is_equal(const fsm& fsm) const {
            return m_current_term == fsm.m_current_term && m_voted_for == fsm.m_voted_for &&
                m_commit_idx == fsm.m_commit_idx &&
                m_last_conf_idx == fsm.m_log.last_conf_idx() &&
                m_last_term == fsm.m_log.last_term() &&
                m_abort_leadership_transfer == fsm.m_abort_leadership_transfer;
        }
        void advance(const fsm& fsm) {
            m_current_term =fsm.m_current_term;
            m_voted_for = fsm.m_voted_for;
            m_commit_idx = fsm.m_commit_idx;
            m_last_conf_idx = fsm.m_log.last_conf_idx();
            m_last_term = fsm.m_log.last_term();
            m_abort_leadership_transfer = fsm.m_abort_leadership_transfer;
        }
    } m_observed;
    fsm_output m_output;
    logical_clock m_clock;
    logical_clock::time_point m_last_election_time = logical_clock::min();
    logical_clock::duration m_randomized_election_timeout = ELECTION_TIMEOUT + logical_clock::duration{1};
private:
    std::vector<std::pair<server_id, rpc_message>> m_messages;
    co_context::mutex m_events_mtx;
    co_context::condition_variable m_events;
    void maybe_commit();
    bool is_past_election_timeout() const {
        return election_elapsed() >= m_randomized_election_timeout;
    }

    template<typename Message>
    void send_to(server_id to, Message&& m) {
        m_messages.push_back(std::make_pair(to, std::move(m)));
        m_events.notify_all();
    }
    void update_current_term(term_t current_term);

    void become_leader();
    void become_candidate(bool is_prevote, bool is_leadership_transfer = false  );
    void become_follower(server_id leader);
    void replicate_to(follower_progress& progress, bool allow_empty);
    void replicate();
    void append_entries(server_id from, append_request&& append_request);

    void append_entries_reply(server_id from, append_reply&& reply);
    void request_vote(server_id from, vote_request&& vote_request);
    void request_vote_reply(server_id from, vote_reply&& vote_reply);
    void install_snapshot_reply(server_id from, snapshot_reply&& reply);
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
    read_id next_read_id() {
        assert(is_leader());
        ++leader_state().last_read_id;
        leader_state().last_read_id_changed = true;
        m_events.notify_all();
        return leader_state().last_read_id;
    }
    // Send read_quorum message to all voting members
    void broadcast_read_quorum(read_id);

    // Process received read_quorum_reply on a leader
    void handle_read_quorum_reply(server_id, const read_quorum_reply&);
public:
    explicit fsm(server_id id, term_t current_term, server_id voted_for, log log,
            index_t commit_idx, failure_detector& failure_detector, fsm_config conf);

    explicit fsm(server_id id, term_t current_term, server_id voted_for, log log, failure_detector& failure_detector, fsm_config conf);

    bool is_leader() const {
        return std::holds_alternative<leader>(m_state);
    }
    bool is_follower() const {
        return std::holds_alternative<follower>(m_state);
    }
    bool is_candidate() const {
        return std::holds_alternative<candidate>(m_state);
    }
    bool is_prevote_candidate() const {
        return is_candidate() && std::get<candidate>(m_state).is_prevote;
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
    index_t log_last_snapshot_idx() const {
        return m_log.get_snapshot().idx;
    }
    index_t log_last_conf_idx() const {
        return m_log.last_conf_idx();
    }

    // Return the last configuration entry with index smaller than or equal to `idx`.
    // Precondition: `log_last_idx()` >= `idx` >= `log_last_snapshot_idx()`.
    const configuration& log_last_conf_for(index_t idx) const {
        return m_log.last_conf_for(idx);
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
    // Ask to search for a leader if one is not known.
    void ping_leader() {
        assert(!current_leader());
        m_ping_leader = true;
    }
    const configuration& get_configuration() const;

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
    void transfer_leadership(logical_clock::duration timeout = logical_clock::duration(0));

    term_t get_current_term() const {
        return m_current_term;
    }

    // How much time has passed since last election or last
    // time we heard from a valid leader.
    logical_clock::duration election_elapsed() const {
        return m_clock.now() - m_last_election_time;
    }
    bool apply_snapshot(snapshot_descriptor snp, size_t max_trailing_entries, size_t max_trailing_bytes, bool local);

    std::optional<std::pair<read_id, index_t>> start_read_barrier(server_id requester);

    server_id id() const { return m_id; }
};

template <typename Message>
void fsm::step(server_id from, const leader& s, Message&& msg) {
    if constexpr (std::is_same_v<Message, append_request>) {
        // We are here if we got AppendEntries RPC with our term
        // but this is impossible since we are the leader and
        // locally applied entries do not go via the RPC. Just ignore it.
    } else if constexpr (std::is_same_v<Message, append_reply>) {
        append_entries_reply(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, vote_request>) {
        request_vote(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, install_snapshot>) {
        send_to(from, snapshot_reply{.current_term = m_current_term,
                     .success = false });
    } else if constexpr (std::is_same_v<Message, snapshot_reply>) {
        install_snapshot_reply(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, read_quorum_reply>) {
        handle_read_quorum_reply(from, msg);
    }
}

template <typename Message>
void fsm::step(server_id from, const candidate& c, Message&& msg) {
    if constexpr (std::is_same_v<Message, vote_request>) {
        request_vote(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, vote_reply>) {
        request_vote_reply(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, install_snapshot>) {
        send_to(from, snapshot_reply{.current_term = m_current_term,
                     .success = false });
    }
}

template <typename Message>
void fsm::step(server_id from, const follower& c, Message&& msg) {
    if constexpr (std::is_same_v<Message, append_request>) {
        append_entries(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, vote_request>) {
        request_vote(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, install_snapshot>) {
        send_to(from, snapshot_reply{.current_term = m_current_term,
                    .success = apply_snapshot(std::move(msg.snp), 0, 0, false)});
    } else if constexpr (std::is_same_v<Message, timeout_now>) {
        // Leadership transfers never use pre-vote; we know we are not
        // recovering from a partition so there is no need for the
        // extra round trip.
        become_candidate(false, true);
    } else if constexpr (std::is_same_v<Message, read_quorum>) {
        advance_commit_idx(msg.leader_commit_idx);
        send_to(from, read_quorum_reply{m_current_term, m_commit_idx, msg.id});
    }
}

template <typename Message>
void fsm::step(server_id from, Message&& msg) {
    if (from == m_id) {
        // on_internal_error(logger, "fsm cannot process messages from itself");
    }
    static_assert(std::is_rvalue_reference<decltype(msg)>::value, "must be rvalue");
    // 4.1. Safety
    // Servers process incoming RPC requests without consulting
    // their current configurations.

    // 3.3. Raft basics.
    //
    // Current terms are exchanged whenever servers
    // communicate; if one server’s current term is smaller
    // than the other’s, then it updates its current term to
    // the larger value. If a candidate or leader discovers
    // that its term is out of date, it immediately reverts to
    // follower state. If a server receives a request with
    // a stale term number, it rejects the request.
    if (msg.current_term > m_current_term) {
        server_id leader{};

        // logger.trace("{} [term: {}] received a message with higher term from {} [term: {}]",
        //     _my_id, _current_term, from, msg.current_term);

        if constexpr (std::is_same_v<Message, append_request> ||
                      std::is_same_v<Message, install_snapshot> ||
                      std::is_same_v<Message, read_quorum>) {
            leader = from;
        } else if constexpr (std::is_same_v<Message, read_quorum_reply> ) {
            // Got a reply to read barrier with higher term. This should not happen.
            // Log and ignore
            // logger.error("{} [term: {}] ignoring read barrier reply with higher term {}",
            //     _my_id, _current_term, msg.current_term);
            return;
        }

        bool ignore_term = false;
        if constexpr (std::is_same_v<Message, vote_request>) {
            // Do not update term on prevote request
            ignore_term = msg.is_prevote;
        } else if constexpr (std::is_same_v<Message, vote_reply>) {
            // We send pre-vote requests with a term in our future. If the
            // pre-vote is granted, we will increment our term when we get a
            // quorum. If it is not, the term comes from the node that
            // rejected our vote so we should become a follower at the new
            // term.
            ignore_term = msg.is_prevote && msg.vote_granted;
        }

        if (!ignore_term) {
            become_follower(leader);
            update_current_term(msg.current_term);
        }
    } else if (msg.current_term < m_current_term) {
        if constexpr (std::is_same_v<Message, append_request> || std::is_same_v<Message, read_quorum>) {
            // Instructs the leader to step down.
            append_reply reply{m_current_term, m_commit_idx, append_reply::rejected{index_t{}, m_log.last_idx()}};
            send_to(from, std::move(reply));
        } else if constexpr (std::is_same_v<Message, install_snapshot>) {
            send_to(from, snapshot_reply{.current_term = m_current_term,
                    .success = false});
        } else if constexpr (std::is_same_v<Message, vote_request>) {
            if (msg.is_prevote) {
                send_to(from, vote_reply{m_current_term, false, true});
            }
        } else {
            // Ignore other cases
            // logger.trace("{} [term: {}] ignored a message with lower term from {} [term: {}]",
            //     m_id, m_current_term, from, msg.current_term);
        }
        return;

    } else /* _current_term == msg.current_term */ {
        if constexpr (std::is_same_v<Message, append_request> ||
                      std::is_same_v<Message, install_snapshot> ||
                      std::is_same_v<Message, read_quorum>) {
            if (is_candidate()) {
                // 3.4 Leader Election
                // While waiting for votes, a candidate may receive an AppendEntries
                // RPC from another server claiming to be leader. If the
                // leader’s term (included in its RPC) is at least as large as the
                // candidate’s current term, then the candidate recognizes the
                // leader as legitimate and returns to follower state.
                become_follower(from);
            } else if (current_leader() == server_id{}) {
                // Earlier we changed our term to match a candidate's
                // term. Now we get the first message from the
                // newly elected leader. Keep track of the current
                // leader to avoid starting an election if the
                // leader becomes idle.
                follower_state().current_leader_id = from;
                m_ping_leader = false;
            }

            // 3.4. Leader election
            // A server remains in follower state as long as it receives
            // valid RPCs from a leader.
            m_last_election_time = m_clock.now();

            if (current_leader() != from) {
                // on_internal_error_noexcept(logger, "Got append request/install snapshot/read_quorum from an unexpected leader");
            }
        }
    }

    auto visitor = [this, from, msg = std::move(msg)](const auto& state) mutable {
        this->step(from, state, std::move(msg));
    };

    std::visit(visitor, m_state);
}

}
