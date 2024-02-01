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

// template <typename Message>
// void fsm::step(server_id from, Message &&msg)
// {
//     // rpc_server收到rpc请求后，第一个调这里
//     if (from == m_id) {
//         return;
//     }
//     if (msg.current_term > m_current_term) {
//         // 说明当前我们落后啦，更新leader
//         server_id leader {};
//         if constexpr (std::is_same_v<Message, append_request>) {
//             leader = from;
//         };
//         become_follower(leader);
//         update_current_term(msg.current_term);
//     } else if (msg.current_term < m_current_term) {
//         if constexpr (std::is_same_v<Message, append_request>) {
//             append_reply reply {

//             };
//             send_to(from, std::move(reply));
//         }
//         return;
//     } else {
//         // msg.current_term == m_current_term
//         if constexpr (std::is_same_v<Message, append_request>) {
//             if (is_candidate()) {
//                 become_follower(from);
//             } else if (current_leader() == server_id {}) {
//                 follower_state().current_leader_id = from;
//             }
//             m_last_election_time = m_clock.now();
//             if (current_leader() != from) {

//             }
//         }
//     }
//     auto visitor = [this, from, msg = std::move(msg)](const auto& state) mutable {
//         this->step(from, state, std::move(msg));
//     };
//     std::visit(visitor, m_state);
// }

// void fsm::append_entries(server_id from, append_request &&append_request)
// {
//     auto [match, term] = m_log.match_term(append_request.prev_log_idx, append_request.prev_log_term);
//     if (!match) {
//         send_to(from, append_reply{
//             m_current_term,
//             // append_reply::rejected{},
//         });
//         return;
//     }
//     index_t last_new_idx = append_request.prev_log_idx;
//     if (!append_request.entries.empty()) {
//         m_log.maybe_append(std::move(append_request.entries));
//     }
//     advance_commit_idx(std::min(append_request.leader_commit_idx, last_new_idx));
//     send_to(from, append_reply {
//         m_current_term,
//         // append_reply::accepted{},
//     });
// }

// void fsm::append_entries_reply(server_id from, append_reply &&reply) {
//     follower_progress* opt_progress = leader_state().m_tracker.find(from);
//     if (opt_progress == nullptr) {
//         return;
//     }
//     follower_progress& progress = *opt_progress;
//     progress.m_commit_idx = std::max(progress.m_commit_idx, reply.commit_idx);
//     if (std::holds_alternative<append_reply::accepted>(reply.result)) {
//         index_t last_idx = std::get<append_reply::accepted>(reply.result).last_new_idx;
//         progress.accepted(last_idx);
        
//         maybe_commit();
        
//     } else {

//     };
// }

// void fsm::request_vote(server_id from, vote_request &&vote_request) {
//     bool can_vote = m_voted_for == from || (m_voted_for == server_id {} && current_leader() == server_id {});
//     if (can_vote && m_log.is_up_to_date(vote_request.last_log_idx, vote_request.last_log_term)) {
//         // 可以投
//         send_to(from, vote_reply {
//             vote_request.current_term,
//             true,
//         });
//     } else {
//         send_to(from, vote_reply {
//             m_current_term,
//             false
//         });
//     }
// }

// void fsm::request_vote_reply(server_id from, vote_reply &&vote_reply) {
//     auto &state = std::get<candidate>(m_state);
//     state.votes_.register_vote(from, vote_reply.vote_granted);
//     switch (state.votes_.tally_votes()) {
//     case vote_result::UNKNOWN:
//         break;
//     case vote_result::WON:
//         become_leader();
//         break;
//     case vote_result::LOST:
//         become_follower(server_id{});
//         break;
//     }
// }

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

const configuration& fsm::get_configuration() const {
    return m_log.get_configuration();
}


void fsm::become_leader() {
    assert(!std::holds_alternative<leader>(m_state));
    m_output.state_changed = true;
    m_state.emplace<leader>(*this);

    // The semaphore is not used on the follower, so the limit could
    // be temporarily exceeded here, and the value of
    // the counter in the semaphore could become negative.
    // This is not a problem though as applier_fiber triggers a snapshot
    // if memory usage approaches the limit.
    // As _applied_idx moves forward, snapshots will eventually release
    // sufficient memory for at least one waiter (add_entry) to proceed.
    // The amount of memory used by log::apply_snapshot for trailing items
    // is limited by the condition
    // _config.snapshot_trailing_size <= _config.max_log_size - max_command_size,
    // which means that at least one command will eventually return from semaphore::wait.
    // leader_state().log_limiter_semaphore->consume(_log.memory_usage());

    m_last_election_time = m_clock.now();
    m_ping_leader = false;
    // a new leader needs to commit at lease one entry to make sure that
    // all existing entries in its log are committed as well. Also it should
    // send append entries RPC as soon as possible to establish its leadership
    // (3.4). Do both of those by committing a dummy entry.
    add_entry(log_entry::dummy());
    // set_configuration() begins replicating from the last entry
    // in the log.
    leader_state().m_tracker.set_configuration(m_log.get_configuration(), m_log.last_idx());
    // logger.trace("fsm::become_leader() {} stable index: {} last index: {}",
    //     _my_id, _log.stable_idx(), _log.last_idx());

}

void fsm::become_candidate(bool is_prevote, bool is_leadership_transfer) {
    if (!std::holds_alternative<candidate>(m_state)) {
        m_output.state_changed = true;
    }
    // When starting a campaign we need to reset current leader otherwise
    // disruptive server prevention will stall an election if quorum of nodes
    // start election together since each one will ignore vote requests from others

    // Note that current state should be destroyed only after the new one is
    // assigned. The exchange here guarantis that.
    std::exchange(m_state, candidate(m_log.get_configuration(), is_prevote));

    reset_election_timeout();

    // 3.4 Leader election
    //
    // A possible outcome is that a candidate neither wins nor
    // loses the election: if many followers become candidates at
    // the same time, votes could be split so that no candidate
    // obtains a majority. When this happens, each candidate will
    // time out and start a new election by incrementing its term
    // and initiating another round of RequestVote RPCs.
    m_last_election_time = m_clock.now();

    auto& votes = candidate_state().votes_;

    const auto& voters = votes.voters();
    if (!voters.contains(m_id)) {
        // We're not a voter in the current configuration (perhaps we completely left it).
        //
        // But sometimes, if we're transitioning between configurations
        // such that we were a voter in the previous configuration, we may still need
        // to become a candidate: the new configuration may be unable to proceed without us.
        //
        // For example, if Cold = {A, B}, Cnew = {B}, A is a leader, switching from Cold to Cnew,
        // and Cnew wasn't yet received by B, then B won't be able to win an election:
        // B will ask A for a vote because it is still in the joint configuration
        // and A won't grant it because B has a shorter log. A is the only node
        // that can become a leader at this point.
        //
        // However we can easily determine when we don't need to become a candidate:
        // If Cnew is already committed, that means that a quorum in Cnew had to accept
        // the Cnew entry, so there is a quorum in Cnew that can proceed on their own.
        //
        // Ref. Raft PhD 4.2.2.
        if (m_log.last_conf_idx() <= m_commit_idx) {
            // Cnew already committed, no need to become a candidate.
            become_follower(server_id{});
            return;
        }

        // The last configuration is not committed yet.
        // This means we must still have access to the previous configuration.
        // Become a candidate only if we were previously a voter.
        auto prev_cfg = m_log.get_prev_configuration();
        assert(prev_cfg);
        if (!prev_cfg->can_vote(m_id)) {
            // We weren't a voter before.
            become_follower(server_id{});
            return;
        }
    }

    term_t term{m_current_term + 1};
    if (!is_prevote) {
        update_current_term(term);
    }
    // Replicate RequestVote
    for (const auto& server : voters) {
        if (server.id == m_id) {
            // Vote for self.
            votes.register_vote(server.id, true);
            if (!is_prevote) {
                // Only record real votes
                m_voted_for = m_id;
            }
            // Already signaled _sm_events in update_current_term()
            continue;
        }
        // logger.trace("{} [term: {}, index: {}, last log term: {}{}{}] sent vote request to {}",
        //     _my_id, term, _log.last_idx(), _log.last_term(), is_prevote ? ", prevote" : "",
        //     is_leadership_transfer ? ", force" : "", server.id);

        send_to(server.id, vote_request{term, m_log.last_idx(), m_log.last_term(), is_prevote, is_leadership_transfer});
    }
    if (votes.tally_votes() == vote_result::WON) {
        // A single node cluster.
        if (is_prevote) {
            // logger.trace("become_candidate[{}] won prevote", _my_id);
            become_candidate(false);
        } else {
            // logger.trace("become_candidate[{}] won vote", _my_id);
            become_leader();
        }
    }
}

void fsm::become_follower(server_id leader) {
    // if (leader == _my_id) {
    //     on_internal_error(logger, "fsm cannot become a follower of itself");
    // }
    if (!std::holds_alternative<follower>(m_state)) {
        m_output.state_changed = true;
    }
    // Note that current state should be destroyed only after the new one is
    // assigned. The exchange here guarantis that.
    std::exchange(m_state, follower{.current_leader_id = leader});
    if (leader != server_id{}) {
        m_ping_leader = false;
        m_last_election_time = m_clock.now();
    }
}
template <typename T>
const log_entry &fsm::add_entry(T command) {
    if (leader_state().m_stepdown) {
        throw not_a_leader({});
    }

    if constexpr (std::is_same_v<T, configuration>) {
        // Do not permit changes which would render the cluster
        // unusable, such as transitioning to an empty configuration or
        // one with no voters.
        // raft::configuration::check(command.current);
        if (m_log.last_conf_idx() > m_commit_idx ||
            m_log.get_configuration().is_joint()) {
            // 4.1. Cluster membership changes/Safety.
            //
            // Leaders avoid overlapping configuration changes by
            // not beginning a new change until the previous
            // change’s entry has committed. It is only safe to
            // start another membership change once a majority of
            // the old cluster has moved to operating under the
            // rules of C_new.
            // logger.trace("[{}] A{}configuration change at index {} is not yet committed (config {}) (commit_idx: {})",
            //     _my_id, _log.get_configuration().is_joint() ? " joint " : " ",
            //     _log.last_conf_idx(), _log.get_configuration(), _commit_idx);
            throw conf_change_in_progress();
        }
        // 4.3. Arbitrary configuration changes using joint consensus
        //
        // When the leader receives a request to change the
        // configuration from C_old to C_new , it stores the
        // configuration for joint consensus (C_old,new) as a log
        // entry and replicates that entry using the normal Raft
        // mechanism.
        configuration tmp(m_log.get_configuration());
        tmp.enter_joint(command.m_current);
        command = std::move(tmp);

        // logger.trace("[{}] appending joint config entry at {}: {}", _my_id, _log.next_idx(), command);
    }

    // utils::get_local_injector().inject("fsm::add_entry/test-failure",
    //                                    [] { throw std::runtime_error("fsm::add_entry/test-failure"); });
    m_log.emplace_back(std::make_shared<log_entry>(log_entry{.term = m_current_term, .idx = m_log.next_idx(), .data = std::move(command)}));
    m_events.notify_all();

    if constexpr (std::is_same_v<T, configuration>) {
        // 4.1. Cluster membership changes/Safety.
        //
        // The new configuration takes effect on each server as
        // soon as it is added to that server’s log: the C_new
        // entry is replicated to the C_new servers, and
        // a majority of the new configuration is used to
        // determine the C_new entry’s commitment.
        leader_state().m_tracker.set_configuration(m_log.get_configuration(), m_log.last_idx());
    }

    return *m_log[m_log.last_idx()];
}

template const log_entry& fsm::add_entry(command_t command);
template const log_entry& fsm::add_entry(configuration command);
template const log_entry& fsm::add_entry(log_entry::dummy dummy);



void fsm::advance_commit_idx(index_t leader_commit_idx) {

    auto new_commit_idx = std::min(leader_commit_idx, m_log.last_idx());

    // logger.trace("advance_commit_idx[{}]: leader_commit_idx={}, new_commit_idx={}",
    //     _my_id, leader_commit_idx, new_commit_idx);

    if (new_commit_idx > m_commit_idx) {
        m_commit_idx = new_commit_idx;
        m_events.notify_all();
        // logger.trace("advance_commit_idx[{}]: signal apply_entries: committed: {}",
        //     _my_id, _commit_idx);
    }
}


void fsm::update_current_term(term_t current_term)
{
    // assert(_current_term < current_term);
    m_current_term = current_term;
    m_voted_for = server_id{};
}

void fsm::reset_election_timeout() {
    // static thread_local std::default_random_engine re{std::random_device{}()};
    // static thread_local std::uniform_int_distribution<> dist;
    // Timeout within range of [1, conf size]
    // m_randomized_election_timeout = ELECTION_TIMEOUT + logical_clock::duration{dist(re,
    //         std::uniform_int_distribution<int>::param_type{1,
    //                 std::max((size_t) ELECTION_TIMEOUT.count(),
    //                         _log.get_configuration().current.size())})};
}



void fsm::replicate_to(follower_progress& progress, bool allow_empty) {

    // logger.trace("replicate_to[{}->{}]: called next={} match={}",
    //     _my_id, progress.id, progress.next_idx, progress.match_idx);

    while (progress.can_send_to()) {
        index_t next_idx = progress.m_next_idx;
        if (progress.m_next_idx > m_log.last_idx()) {
            next_idx = index_t(0);
            // logger.trace("replicate_to[{}->{}]: next past last next={} stable={}, empty={}",
            //         _my_id, progress.id, progress.next_idx, _log.last_idx(), allow_empty);
            if (!allow_empty) {
                // Send out only persisted entries.
                return;
            }
        }

        allow_empty = false; // allow only one empty message

        // A log containing a snapshot, a few trailing entries and
        // a few new entries may look like this:
        // E - log entry
        // S_idx - snapshot index
        // E_i1 E_i2 E_i3 Ei_4 E_i5 E_i6
        //      ^
        //      S_idx = i2
        // If the follower's next_idx is i1 we need to
        // enter snapshot transfer mode even when we have
        // i1 in the log, since it is not possible to get the term of
        // the entry previous to i1 and verify that the follower's tail
        // contains no uncommitted entries.
        index_t prev_idx = progress.m_next_idx - index_t{1};
        std::optional<term_t> prev_term = m_log.term_for(prev_idx);
        if (!prev_term) {
            const snapshot_descriptor& snapshot = m_log.get_snapshot();
            // We need to transfer the snapshot before we can
            // continue syncing the log.
            progress.become_snapshot(snapshot.idx);
            send_to(progress.m_server_id, install_snapshot{m_current_term, snapshot});
            // logger.trace("replicate_to[{}->{}]: send snapshot next={} snapshot={}",
            //         _my_id, progress.id, progress.next_idx,  snapshot.idx);
            return;
        }

        append_request req = {
            .current_term = m_current_term,
            .prev_log_idx = prev_idx,
            .prev_log_term = prev_term.value(),
            .leader_commit_idx = m_commit_idx,
            .entries = std::vector<log_entry_ptr>()
        };

        if (next_idx) {
            size_t size = 0;
            while (next_idx <= m_log.last_idx() && size < m_config.append_request_threshold) {
                const auto& entry = m_log[next_idx];
                req.entries.push_back(entry);
                // logger.trace("replicate_to[{}->{}]: send entry idx={}, term={}",
                //              _my_id, progress.id, entry->idx, entry->term);
                // size += entry_size(*entry);
                next_idx++;
                if (progress.state == follower_progress::state::PROBE) {
                    break; // in PROBE mode send only one entry
                }
            }

            if (progress.state == follower_progress::state::PIPELINE) {
                progress.in_flight++;
                // Optimistically update next send index. In case
                // a message is lost there will be negative reply that
                // will re-send idx.
                progress.m_next_idx = next_idx;
            }
        } else {
            // logger.trace("replicate_to[{}->{}]: send empty", _my_id, progress.id);
        }

        send_to(progress.m_server_id, std::move(req));

        if (progress.state == follower_progress::state::PROBE) {
             progress.probe_sent = true;
        }
    }
}

void fsm::replicate() {
    for (auto& [id, progress] : leader_state().m_tracker) {
        if (progress.m_server_id != m_id) {
            replicate_to(progress, false);
        }
    }
}

co_context::task<fsm_output> fsm::poll_output() {
    // logger.trace("fsm::poll_output() {} stable index: {} last index: {}",
    //     _my_id, _log.stable_idx(), _log.last_idx());

    while (true) {
        auto diff = m_log.last_idx() - m_log.stable_idx();

        if (diff > 0 || !m_messages.empty() || !m_observed.is_equal(*this) || m_output.max_read_id_with_quorum ||
                (is_leader() && leader_state().last_read_id_changed) || m_output.snp || !m_output.snps_to_drop.empty() || m_output.state_changed) {
            break;
        }
        co_await m_events_mtx.lock();
        co_await m_events.wait(m_events_mtx);
    }
    // while (utils::get_local_injector().enter("fsm::poll_output/pause")) {
    //     co_await seastar::sleep(std::chrono::milliseconds(100));
    // }
    co_return get_output();
    
}

fsm_output fsm::get_output() {
    auto diff = m_log.last_idx() - m_log.stable_idx();

    if (is_leader()) {
        // send delayed read quorum requests if any
        if (leader_state().last_read_id_changed) {
            broadcast_read_quorum(leader_state().last_read_id);
            leader_state().last_read_id_changed = false;
        }
        // replicate accumulated entries
        if (diff) {
            replicate();
        }
    }

    fsm_output output = std::exchange(m_output, fsm_output{});

    if (diff > 0) {
        output.log_entries.reserve(diff);

        for (auto i = m_log.stable_idx() + 1; i <= m_log.last_idx(); i++) {
            // Copy before saving to storage to prevent races with log updates,
            // e.g. truncation of the log.
            // TODO: avoid copies by making sure log truncate is
            // copy-on-write.
            output.log_entries.emplace_back(m_log[i]);
        }
    }

    if (m_observed.m_current_term != m_current_term || m_observed.m_voted_for != m_voted_for) {
        output.term_and_vote = {m_current_term, m_voted_for};
    }

    // Return committed entries.
    // Observer commit index may be smaller than snapshot index
    // in which case we should not attempt committing entries belonging
    // to a snapshot.
    auto observed_ci =  std::max(m_observed.m_commit_idx, m_log.get_snapshot().idx);
    if (observed_ci < m_commit_idx) {
        output.committed_entries.reserve(m_commit_idx - observed_ci);

        for (auto idx = observed_ci + 1; idx <= m_commit_idx; ++idx) {
            const auto& entry = m_log[idx];
            output.committed_entries.push_back(entry);
        }
    }

    // Get a snapshot of all unsent messages.
    // Do it after populating log_entries and committed arrays
    // to not lose messages in case arrays population throws
    std::swap(output.messages, m_messages);

    // Get status of leadership transfer (if any)
    output.abort_leadership_transfer = std::exchange(m_abort_leadership_transfer, false);

    // Fill server_address_set corresponding to the configuration from
    // the rpc point of view.
    //
    // Effective rpc configuration changes when one of the following applies:
    // * `last_conf_idx()` could have changed or
    // * A new configuration entry may be overwritten by application of two
    //   snapshots with different configurations following each other.
    // * Leader overwrites a follower's log.
    if (m_observed.m_last_conf_idx != m_log.last_conf_idx() ||
            (m_observed.m_current_term != m_log.last_term() &&
             m_observed.m_last_term != m_log.last_term())) {
        configuration last_log_conf = m_log.get_configuration();
        last_log_conf.m_current.merge(last_log_conf.m_previous);
        output.configuration = last_log_conf.m_current;
    }

    // Advance the observed state.
    m_observed.advance(*this);

    // Be careful to do that only after any use of stable_idx() in this
    // function and after any code that may throw
    if (output.log_entries.size()) {
        // We advance stable index before the entries are
        // actually persisted, because if writing to stable storage
        // will fail the FSM will be stopped and get_output() will
        // never be called again, so any new state that assumes that
        // the entries are stable will not be observed.
        advance_stable_idx(output.log_entries.back()->idx);
    }

    return output;
}

void fsm::advance_stable_idx(index_t idx) {
    index_t prev_stable_idx = m_log.stable_idx();
    m_log.stable_to(idx);
    // logger.trace("advance_stable_idx[{}]: prev_stable_idx={}, idx={}", _my_id, prev_stable_idx, idx);
    if (is_leader()) {
        auto leader_progress = leader_state().m_tracker.find(m_id);
        if (leader_progress) {
            // If this server is leader and is part of the current
            // configuration, update it's progress and optionally
            // commit new entries.
            leader_progress->accepted(idx);
            maybe_commit();
        }
    }
}

void fsm::tick_leader() {
    if (election_elapsed() >= ELECTION_TIMEOUT) {
        // 6.2 Routing requests to the leader
        // A leader in Raft steps down if an election timeout
        // elapses without a successful round of heartbeats to a majority
        // of its cluster; this allows clients to retry their requests
        // with another server.
        return become_follower(server_id{});
    }

    auto& state = leader_state();
    auto active = state.m_tracker.get_activity_tracker();
    active(m_id); // +1 for self
    for (auto& [id, progress] : state.m_tracker) {
        if (progress.m_server_id != m_id) {
            if (m_failure_detector.is_alive(progress.m_server_id)) {
                active(progress.m_server_id);
            }
            switch(progress.state) {
            case follower_progress::state::PROBE:
                // allow one probe to be resent per follower per time tick
                progress.probe_sent = false;
                break;
            case follower_progress::state::PIPELINE:
                if (progress.in_flight == follower_progress::max_in_flight) {
                    progress.in_flight--; // allow one more packet to be sent
                }
                break;
            case follower_progress::state::SNAPSHOT:
                continue;
            }
            if (progress.m_match_idx < m_log.last_idx() || progress.m_commit_idx < m_commit_idx) {
                // logger.trace("tick[{}]: replicate to {} because match={} < last_idx={} || "
                //     "follower commit_idx={} < commit_idx={}",
                //     _my_id, progress.id, progress.match_idx, _log.last_idx(),
                //     progress.commit_idx, _commit_idx);

                replicate_to(progress, true);
            }
        }
    }
    if (state.last_read_id != state.max_read_id_with_quorum) {
        // Re-send last read barrier to ensure forward progress in the face of packet loss
        broadcast_read_quorum(state.last_read_id);
    }
    if (active) {
        // Advance last election time if we heard from
        // the quorum during this tick.
        m_last_election_time = m_clock.now();
    }

    if (state.m_stepdown) {
        // logger.trace("tick[{}]: stepdown is active", _my_id);
        auto me = leader_state().m_tracker.find(m_id);
        if (me == nullptr || !me->m_can_vote) {
            // logger.trace("tick[{}]: not aborting stepdown because we have been removed from the configuration", _my_id);
            // Do not abort stepdown if not part of the current
            // config or non voting member since the node cannot
            // be a leader any longer
        } else if (*state.m_stepdown <= m_clock.now()) {
            // logger.trace("tick[{}]: cancel stepdown", _my_id);
            // Cancel stepdown (only if the leader is part of the cluster)
            // leader_state().log_limiter_semaphore->signal(_config.max_log_size);
            state.m_stepdown.reset();
            state.timeout_now_sent.reset();
            m_abort_leadership_transfer = true;
            m_events.notify_all(); // signal to handle aborting of leadership transfer
        } else if (state.timeout_now_sent) {
            // logger.trace("tick[{}]: resend timeout_now", _my_id);
            // resend timeout now in case it was lost
            send_to(*state.timeout_now_sent, timeout_now{m_current_term});
        }
    }
}

void fsm::tick() {
    m_clock.advance();
    
}


void fsm::append_entries(server_id from, append_request&& request) {
    // logger.trace("append_entries[{}] received ct={}, prev idx={} prev term={} commit idx={}, idx={} num entries={}",
    //         _my_id, request.current_term, request.prev_log_idx, request.prev_log_term,
    //         request.leader_commit_idx, request.entries.size() ? request.entries[0]->idx : index_t(0), request.entries.size());

    assert(is_follower());

    // Ensure log matching property, even if we append no entries.
    // 3.5
    // Until the leader has discovered where it and the
    // follower’s logs match, the leader can send
    // AppendEntries with no entries (like heartbeats) to save
    // bandwidth.
    auto [match, term] = m_log.match_term(request.prev_log_idx, request.prev_log_term);
    if (!match) {
        // logger.trace("append_entries[{}]: no matching term at position {}: expected {}, found {}",
        //         _my_id, request.prev_log_idx, request.prev_log_term, term);
        // Reply false if log doesn't contain an entry at
        // prevLogIndex whose term matches prevLogTerm (§5.3).
        send_to(from, append_reply{m_current_term, m_commit_idx, append_reply::rejected{request.prev_log_idx, m_log.last_idx()}});
        return;
    }

    // If there are no entries it means that the leader wants
    // to ensure forward progress. Reply with the last index
    // that matches.
    index_t last_new_idx = request.prev_log_idx;

    if (!request.entries.empty()) {
        last_new_idx = m_log.maybe_append(std::move(request.entries));
    }

    // Do not advance commit index further than last_new_idx, or we could incorrectly
    // mark outdated entries as committed (see #9965).
    advance_commit_idx(std::min(request.leader_commit_idx, last_new_idx));

    send_to(from, append_reply{m_current_term, m_commit_idx, append_reply::accepted{last_new_idx}});
}

void fsm::append_entries_reply(server_id from, append_reply&& reply) {
    assert(is_leader());

    follower_progress* opt_progress = leader_state().m_tracker.find(from);
    if (opt_progress == nullptr) {
        // A message from a follower removed from the
        // configuration.
        return;
    }
    follower_progress& progress = *opt_progress;

    if (progress.state == follower_progress::state::PIPELINE) {
        if (progress.in_flight) {
            // in_flight is not precise, so do not let it underflow
            progress.in_flight--;
        }
    }

    if (progress.state == follower_progress::state::SNAPSHOT) {
        // logger.trace("append_entries_reply[{}->{}]: ignored in snapshot state", _my_id, from);
        return;
    }

    progress.m_commit_idx = std::max(progress.m_commit_idx, reply.commit_idx);

    if (std::holds_alternative<append_reply::accepted>(reply.result)) {
        // accepted
        index_t last_idx = std::get<append_reply::accepted>(reply.result).last_new_idx;

        // logger.trace("append_entries_reply[{}->{}]: accepted match={} last index={}",
        //     _my_id, from, progress.match_idx, last_idx);

        progress.accepted(last_idx);

        progress.become_pipeline();

        // If a leader is stepping down, transfer the leadership
        // to a first voting node that has fully replicated log.
        if (leader_state().m_stepdown && !leader_state().timeout_now_sent &&
                         progress.m_can_vote && progress.m_match_idx == m_log.last_idx()) {
            // 此时，更新了follower的状态，正巧我在作leader transfer
            // 这时候，我们就需要看看这个刚刚更新的follower是不是最新的了，如果是的，则send time out to it.
            send_timeout_now(progress.m_server_id);
            // We may have resigned leadership if a stepdown process completed
            // while the leader is no longer part of the configuration.
            if (!is_leader()) {
                return;
            }
        }

        // check if any new entry can be committed
        maybe_commit();

        // The call to maybe_commit() may initiate and immediately complete stepdown process
        // so the comment above the provios is_leader() check applies here too. 
        if (!is_leader()) {
            return;
        }
    } else {
        // rejected
        append_reply::rejected rejected = std::get<append_reply::rejected>(reply.result);

        // logger.trace("append_entries_reply[{}->{}]: rejected match={} index={}",
        //     _my_id, from, progress.match_idx, rejected.non_matching_idx);

        // If non_matching_idx and last_idx are zero it means that a follower is looking for a leader
        // as such message cannot be a result of real mismatch.
        // Send an empty append message to notify it that we are the leader
        if (rejected.non_matching_idx == index_t{0} && rejected.last_idx == index_t{0}) {
            // logger.trace("append_entries_reply[{}->{}]: send empty append message", _my_id, from);
            replicate_to(progress, true);
            return;
        }

        // check reply validity
        if (progress.is_stray_reject(rejected)) {
            // logger.trace("append_entries_reply[{}->{}]: drop stray append reject", _my_id, from);
            return;
        }

        // is_stray_reject may return a false negative so even if the check above passes,
        // we may still be dealing with a stray reject. That's fine though; it is always safe
        // to rollback next_idx on a reject and in fact that's what the Raft spec (TLA+) does.
        // Detecting stray rejects is an optimization that should rarely even be needed.

        // Start re-sending from the non matching index, or from
        // the last index in the follower's log.
        // FIXME: make it more efficient
        progress.m_next_idx = std::min(rejected.non_matching_idx, index_t(rejected.last_idx + 1));

        progress.become_probe();

        // By `is_stray_reject(rejected) == false` we know that `rejected.non_matching_idx > progress.match_idx`
        // and `rejected.last_idx + 1 > progress.match_idx`. By the assignment to `progress.next_idx` above, we get:
        assert(progress.m_next_idx > progress.m_match_idx);
    }

    // We may have just applied a configuration that removes this
    // follower, so re-track it.
    opt_progress = leader_state().m_tracker.find(from);
    if (opt_progress != nullptr) {
        // logger.trace("append_entries_reply[{}->{}]: next_idx={}, match_idx={}",
        //     _my_id, from, opt_progress->next_idx, opt_progress->match_idx);

        replicate_to(*opt_progress, false);
    }
}

void fsm::request_vote(server_id from, vote_request&& request) {

    // We can cast a vote in any state. If the candidate's term is
    // lower than ours, we ignore the request. Otherwise we first
    // update our current term and convert to a follower.
    assert(request.is_prevote || m_current_term == request.current_term);

    bool can_vote =
        // We can vote if this is a repeat of a vote we've already cast...
        m_voted_for == from ||
        // ...we haven't voted and we don't think there's a leader yet in this term...
        (m_voted_for == server_id{} && current_leader() == server_id{}) ||
        // ...this is prevote for a future term...
        // (we will get here if the node does not know any leader yet and already
        //  voted for some other node, but now it get even newer prevote request)
        (request.is_prevote && request.current_term > m_current_term);

    // ...and we believe the candidate is up to date.
    if (can_vote && m_log.is_up_to_date(request.last_log_idx, request.last_log_term)) {

        // logger.trace("{} [term: {}, index: {}, last log term: {}, voted_for: {}] "
        //     "voted for {} [log_term: {}, log_index: {}]",
        //     m_id, m_current_term, m_log.last_idx(), m_log.last_term(), m_voted_for,
        //     from, request.last_log_term, request.last_log_idx);
        if (!request.is_prevote) { // Only record real votes
            // If a server grants a vote, it must reset its election
            // timer. See Raft Summary.
            m_last_election_time = m_clock.now();
            m_voted_for = from;
        }
        // The term in the original message and current local term are the
        // same in the case of regular votes, but different for pre-votes.
        //
        // When responding to {Pre,}Vote messages we include the term
        // from the message, not the local term. To see why, consider the
        // case where a single node was previously partitioned away and
        // its local term is now out of date. If we include the local term
        // (recall that for pre-votes we don't update the local term), the
        // (pre-)campaigning node on the other end will proceed to ignore
        // the message (it ignores all out of date messages).
        send_to(from, vote_reply{request.current_term, true, request.is_prevote});
    } else {
        // If a vote is not granted, this server is a potential
        // viable candidate, so it should not reset its election
        // timer, to avoid election disruption by non-viable
        // candidates.
        // logger.trace("{} [term: {}, index: {}, log_term: {}, voted_for: {}] "
        //     "rejected vote for {} [current_term: {}, log_term: {}, log_index: {}, is_prevote: {}]",
        //     _my_id, _current_term, _log.last_idx(), _log.last_term(), _voted_for,
        //     from, request.current_term, request.last_log_term, request.last_log_idx, request.is_prevote);

        send_to(from, vote_reply{m_current_term, false, request.is_prevote});
    }
}

void fsm::request_vote_reply(server_id from, vote_reply&& reply) {
    assert(is_candidate());

    // logger.trace("request_vote_reply[{}] received a {} vote from {}", _my_id, reply.vote_granted ? "yes" : "no", from);

    auto& state = std::get<candidate>(m_state);
    // Should not register a reply to prevote as a real vote
    if (state.is_prevote != reply.is_prevote) {
        // logger.trace("request_vote_reply[{}] ignoring prevote from {} as state is vote", _my_id, from);
        return;
    }
    state.votes_.register_vote(from, reply.vote_granted);

    switch (state.votes_.tally_votes()) {
    case vote_result::UNKNOWN:
        break;
    case vote_result::WON:
        if (state.is_prevote) {
            // logger.trace("request_vote_reply[{}] won prevote", _my_id);
            become_candidate(false);
        } else {
            // logger.trace("request_vote_reply[{}] won vote", _my_id);
            become_leader();
        }
        break;
    case vote_result::LOST:
        become_follower(server_id{});
        break;
    }
}

void fsm::install_snapshot_reply(server_id from, snapshot_reply&& reply) {
    follower_progress* opt_progress= leader_state().m_tracker.find(from);
    // The follower is removed from the configuration.
    if (opt_progress == nullptr) {
        return;
    }
    follower_progress& progress = *opt_progress;

    if (progress.state != follower_progress::state::SNAPSHOT) {
        // logger.trace("install_snapshot_reply[{}]: called not in snapshot state", _my_id);
        return;
    }

    // No matter if snapshot transfer failed or not move back to probe state
    progress.become_probe();

    if (reply.success) {
        // If snapshot was successfully transferred start replication immediately
        replicate_to(progress, false);
    }
    // Otherwise wait for a heartbeat. Next attempt will move us to SNAPSHOT state
    // again and snapshot transfer will be attempted one more time.
}

bool fsm::apply_snapshot(snapshot_descriptor snp, size_t max_trailing_entries, size_t max_trailing_bytes, bool local) {
    // logger.trace("apply_snapshot[{}]: current term: {}, term: {}, idx: {}, id: {}, local: {}",
    //         _my_id, _current_term, snp.term, snp.idx, snp.id, local);
    // If the snapshot is locally generated, all entries up to its index must have been locally applied,
    // so in particular they must have been observed as committed.
    // Remote snapshots are only applied if we're a follower.
    assert((local && snp.idx <= m_observed.m_commit_idx) || (!local && is_follower()));

    // We don't apply snapshots older than the last applied one.
    // Furthermore, for remote snapshots, we can *only* apply them if they are fresher than our commit index.
    // Applying older snapshots could result in out-of-order command application to the replicated state machine,
    // leading to serializability violations.
    const auto& current_snp = m_log.get_snapshot();
    if (snp.idx <= current_snp.idx || (!local && snp.idx <= m_commit_idx)) {
        // logger.error("apply_snapshot[{}]: ignore outdated snapshot {}/{} current one is {}/{}, commit_idx={}",
        //                 _my_id, snp.id, snp.idx, current_snp.id, current_snp.idx, _commit_idx);
        m_output.snps_to_drop.push_back(snp.id);
        return false;
    }

    m_output.snps_to_drop.push_back(current_snp.id);

    // If the snapshot is local, _commit_idx is larger than snp.idx.
    // Otherwise snp.idx becomes the new commit index.
    m_commit_idx = std::max(m_commit_idx, snp.idx);
    m_output.snp.emplace(fsm_output::applied_snapshot{snp, local});
    // size_t units = m_log.apply_snapshot(std::move(snp), max_trailing_entries, max_trailing_bytes);
    if (is_leader()) {
        // logger.trace("apply_snapshot[{}]: signal {} available units", _my_id, units);
        // leader_state().log_limiter_semaphore->signal(units);
    }
    m_events.notify_all();
    return true;
}

void fsm::transfer_leadership(logical_clock::duration timeout) {
    // check_is_leader();
    auto leader = leader_state().m_tracker.find(m_id);
    if (configuration::voter_count(get_configuration().m_current) == 1 && leader && leader->m_can_vote) {
        // If there is only one voter and it is this node we cannot have another node
        // to transfer leadership to
        throw raft::no_other_voting_member();
    }

    leader_state().m_stepdown = m_clock.now() + timeout;
    // Stop new requests from coming in
    // leader_state().log_limiter_semaphore->consume(_config.max_log_size);
    // If there is a fully up-to-date voting replica make it start an election
    for (auto&& [_, p] : leader_state().m_tracker) {
        if (p.m_server_id != m_id && p.m_can_vote && p.m_match_idx == m_log.last_idx()) {
            send_timeout_now(p.m_server_id);
            break;
        }
    }
}

void fsm::send_timeout_now(server_id id) {
    // logger.trace("send_timeout_now[{}] send timeout_now to {}", _my_id, id);
    send_to(id, timeout_now{m_current_term});
    leader_state().timeout_now_sent = id;
    auto me = leader_state().m_tracker.find(m_id);
    if (me == nullptr || !me->m_can_vote) {
        // logger.trace("send_timeout_now[{}] become follower", _my_id);
        become_follower({});
    }
}

void fsm::broadcast_read_quorum(read_id id) {
    // logger.trace("broadcast_read_quorum[{}] send read id {}", _my_id, id);
    for (auto&& [_, p] : leader_state().m_tracker) {
        if (p.m_can_vote) {
            if (p.m_server_id == m_id) {
                handle_read_quorum_reply(m_id, read_quorum_reply{m_current_term, m_commit_idx, id});
            } else {
                send_to(p.m_server_id, read_quorum{m_current_term, std::min(p.m_match_idx, m_commit_idx), id});
            }
        }
    }
}

void fsm::handle_read_quorum_reply(server_id from, const read_quorum_reply& reply) {
    assert(is_leader());
    // logger.trace("handle_read_quorum_reply[{}] got reply from {} for id {}", _my_id, from, reply.id);
    auto& state = leader_state();
    follower_progress* progress = state.m_tracker.find(from);
    if (progress == nullptr) {
        // A message from a follower removed from the
        // configuration.
        return;
    }
    progress->m_commit_idx = std::max(progress->m_commit_idx, reply.commit_idx);
    progress->m_max_acked_read = std::max(progress->m_max_acked_read, reply.id);

    if (reply.id <= state.max_read_id_with_quorum) {
        // We already have a quorum for a more resent id, so no need to recalculate
        return;
    }

    read_id new_committed_read = leader_state().m_tracker.committed(state.max_read_id_with_quorum);

    if (new_committed_read <= state.max_read_id_with_quorum) {
        return; // nothing new is committed
    }

    m_output.max_read_id_with_quorum = state.max_read_id_with_quorum = new_committed_read;

    // logger.trace("handle_read_quorum_reply[{}] new commit read {}", _my_id, new_committed_read);

    m_events.notify_all();
}

std::optional<std::pair<read_id, index_t>> fsm::start_read_barrier(server_id requester) {
    // check_is_leader();

    // Make sure that only a leader or a not that is part of the config can request read barrier
    // Nodes outside of the config may never get the data, so they will not be able to read it.
    if (requester != m_id && leader_state().m_tracker.find(requester) == nullptr) {
        throw std::runtime_error("Read barrier requested by a node outside of the configuration");
    }

    auto term_for_commit_idx = m_log.term_for(m_commit_idx);
    assert(term_for_commit_idx);

    if (*term_for_commit_idx != m_current_term) {
        return {};
    }

    read_id id = next_read_id();
    // logger.trace("start_read_barrier[{}] starting read barrier with id {}", _my_id, id);
    return std::make_pair(id, m_commit_idx);
}

void fsm::stop() {
    if (is_leader()) {
        // Become follower to stop accepting requests
        // (in particular, abort waits on log_limiter_semaphore and prevent new ones).
        become_follower({});
    }
    // _sm_events.broken();
}
}