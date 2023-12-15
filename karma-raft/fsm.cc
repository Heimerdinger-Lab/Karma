#include "karma-raft/fsm.h"
#include "common.h"
#include <memory>
namespace raft {
void fsm::maybe_commit() {
    index_t new_commit_idx = leader_state().m_tracker.committed(m_commit_idx);
    if (new_commit_idx <= m_commit_idx) {
        return;
    }
    bool committed_conf_change = m_commit_idx < m_log.last_conf_idx() && new_commit_idx >= m_log.last_conf_idx();
    if (m_log[new_commit_idx]->term != m_current_term) {
        return;
    }
    m_commit_idx = new_commit_idx;
    m_events.notify_all();
    if (committed_conf_change) {
        if (m_log.get_configuration().is_joint()) {
            configuration cfg(m_log.get_configuration());
            cfg.leave_joint();
            m_log.emplace_back(std::make_shared<log_entry>(log_entry{.term = m_current_term, .idx = m_log.next_idx(), .data = std::move(cfg)}));
            leader_state().m_tracker.set_configuration(m_log.get_configuration(), m_log.last_idx());
            maybe_commit();
        } else {
            auto lp = leader_state().m_tracker.find(m_id);
            if (lp == nullptr || !lp->m_can_vote) {
                transfer_leadership();
            }
        }
        if (is_leader() && leader_state().last_read_id != leader_state().max_read_id_with_quorum) {
            // Since after reconfiguration the quorum will be calculated based on a new config
            // old reads may never get the quorum. Think about reconfiguration from {A, B, C} to
            // {A, D, E}. Since D, E never got read_quorum request they will never reply, so the
            // read will be stuck at least till leader tick. Re-broadcast last request here to expedite
            // its completion
            broadcast_read_quorum(leader_state().last_read_id);
        }
    }
}

template <typename Message>
void fsm::step(server_id from, Message &&msg)
{
    // rpc_server收到rpc请求后，第一个调这里
    if (from == m_id) {
        return;
    }
    if (msg.current_term > m_current_term) {
        // 说明当前我们落后啦，更新leader
        server_id leader {};
        if constexpr (std::is_same_v<Message, append_request>) {
            leader = from;
        };
        become_follower(leader);
        update_current_term(msg.current_term);
    } else if (msg.current_term < m_current_term) {
        if constexpr (std::is_same_v<Message, append_request>) {
            append_reply reply {

            };
            send_to(from, std::move(reply));
        }
        return;
    } else {
        // msg.current_term == m_current_term
        if constexpr (std::is_same_v<Message, append_request>) {
            if (is_candidate()) {
                become_follower(from);
            } else if (current_leader() == server_id {}) {
                follower_state().current_leader_id = from;
            }
            m_last_election_time = m_clock.now();
            if (current_leader() != from) {

            }
        }
    }
    auto visitor = [this, from, msg = std::move(msg)](const auto& state) mutable {
        this->step(from, state, std::move(msg));
    };
    std::visit(visitor, m_state);
}
template <typename Message>
inline void fsm::step(server_id from, const leader &s, Message &&msg) {
    if constexpr (std::is_same_v<Message, append_request>) {

    } else if constexpr (std::is_same_v<Message, append_reply>) {
        append_entries_reply(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, vote_request>) {
        request_vote(from, std::move(msg));
    }
}

template <typename Message>
void fsm::step(server_id from, const candidate &s, Message &&msg) {
    if constexpr (std::is_same_v<Message, vote_request>) {
        request_vote(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, vote_reply>) {
        request_vote_reply(from, std::move(msg));
    } 
}

template <typename Message>
void fsm::step(server_id from, const follower &s, Message &&msg) {
    if constexpr (std::is_same_v<Message, append_request>) {
        append_request(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, vote_request>) {
        request_vote(from, std::move(msg));
    } else if constexpr (std::is_same_v<Message, timeout_now>) {
        become_candidate();
    }
}


void fsm::append_entries(server_id from, append_request &&append_request)
{
    auto [match, term] = m_log.match_term(append_request.prev_log_idx, append_request.prev_log_term);
    if (!match) {
        send_to(from, append_reply{
            m_current_term,
            // append_reply::rejected{},
        });
        return;
    }
    index_t last_new_idx = append_request.prev_log_idx;
    if (!append_request.entries.empty()) {
        m_log.maybe_append(std::move(append_request.entries));
    }
    advance_commit_idx(std::min(append_request.leader_commit_idx, last_new_idx));
    send_to(from, append_reply {
        m_current_term,
        // append_reply::accepted{},
    });
}

void fsm::append_entries_reply(server_id from, append_reply &&reply) {
    follower_progress* opt_progress = leader_state().m_tracker.find(from);
    if (opt_progress == nullptr) {
        return;
    }
    follower_progress& progress = *opt_progress;
    progress.m_commit_idx = std::max(progress.m_commit_idx, reply.commit_idx);
    if (std::holds_alternative<append_reply::accepted>(reply.result)) {
        index_t last_idx = std::get<append_reply::accepted>(reply.result).last_new_idx;
        progress.accepted(last_idx);
        
        maybe_commit();
        
    } else {

    };
}

void fsm::request_vote(server_id from, vote_request &&vote_request) {
    bool can_vote = m_voted_for == from || (m_voted_for == server_id {} && current_leader() == server_id {});
    if (can_vote && m_log.is_up_to_date(vote_request.last_log_idx, vote_request.last_log_term)) {
        // 可以投
        send_to(from, vote_reply {
            vote_request.current_term,
            true,
        });
    } else {
        send_to(from, vote_reply {
            m_current_term,
            false
        });
    }
}

void fsm::request_vote_reply(server_id from, vote_reply &&vote_reply) {
    auto &state = std::get<candidate>(m_state);
    state.votes_.register_vote(from, vote_reply.vote_granted);
    switch (state.votes_.tally_votes()) {
    case vote_result::UNKNOWN:
        break;
    case vote_result::WON:
        become_leader();
        break;
    case vote_result::LOST:
        become_follower(server_id{});
        break;
    }
}

fsm::fsm(server_id id, term_t current_term, server_id voted_for, log log,
        index_t commit_idx, failure_detector& failure_detector, fsm_config config) :
        m_id(id), m_current_term(current_term), m_voted_for(voted_for),
        m_log(std::move(log)), m_failure_detector(failure_detector), m_config(config) {
    if (id == raft::server_id{}) {
        // throw std::invalid_argument("raft::fsm: raft instance cannot have id zero");
    }
    // The snapshot can not contain uncommitted entries
    m_commit_idx = m_log.get_snapshot().idx;
    m_observed.advance(*this);
    // After we observed the state advance commit_idx to persisted one (if provided)
    // so that the log can be replayed
    m_commit_idx = std::max(m_commit_idx, commit_idx);
    // logger.trace("fsm[{}]: starting, current term {}, log length {}, commit index {}", _my_id, _current_term, _log.last_idx(), _commit_idx);

    // Init timeout settings
    if (m_log.get_configuration().m_current.size() == 1 && m_log.get_configuration().can_vote(m_id)) {
        become_candidate(m_config.enable_prevoting);
    } else {
        reset_election_timeout();
    }
}


void fsm::become_leader() {
    m_output.state_changed = true;
    m_state.emplace<leader>(*this);
    m_last_election_time = m_clock.now();
    add_entry(log_entry::dummy{});

}

void fsm::become_candidate() {
    if (!std::holds_alternative<candidate>(m_state)) {
        m_output.state_changed = true;
    }
    // std::exchange(m_state, candidate {});
    m_last_election_time = m_clock.now();
    auto& votes = candidate_state().votes_;
    const auto& voters = votes.voters();
    term_t term {m_current_term + 1};
    update_current_term(term);
    for (const auto& i : voters) {
        // if (i.id == m_id) {
        //     // 给自己投一票
        //     // votes.register_vote(i.id, true);
        //     continue;
        // }
        // send_to(i.id, vote_request {
        //     term,
        //     m_log.last_idx(),
        //     m_log.last_term(),
        // });
    };
    if (votes.tally_votes() == vote_result::WON) {
        become_leader();
    }
}

void fsm::become_follower(server_id leader) {
    if (!std::holds_alternative<follower>(m_state)) {
        m_output.state_changed = true;
    }
    std::exchange(m_state, follower {
        .current_leader_id = leader
    });
    if (leader != server_id {}) {
        m_last_election_time = m_clock.now();
    }
}
template <typename T>
const log_entry &fsm::add_entry(T command) {
    // TODO: 在此处插入 return 语句
    // m_log.emplace_back(std::make_shared<log_entry>(log_entry{m_current_term, m_log.next_idx(), std::move(command)}));
    // m_sm_events.signal();
}

void fsm::replicate_to(bool allow_empty) {
}

void fsm::replicate() {
}

co_context::task<fsm_output> fsm::poll_output() {
    co_await m_events_mtx.lock();
    while (true) {
        auto diff = m_log.last_idx() - m_log.stable_idx();
        if (diff > 0 || m_messages.size() || !m_observed.is_equal(*this) || m_output.state_changed) {
            break;
        }
        co_await m_events.wait(m_events_mtx);
    }
    co_return get_output();
    
}

fsm_output fsm::get_output() {
    
}

void fsm::tick() {
    m_clock.advance();
    
}
}