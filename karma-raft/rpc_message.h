#pragma once
#include "karma-raft/common.h"

namespace raft {
struct ping_pong_request {
    std::string m_msg;
};
struct ping_pong_reply {
    std::string m_msg;
};

struct append_request {
    // The leader's term.
    term_t current_term;
    // The leader's ID
    server_id leader_id;
    
    // Index of the log entry immediately preceding new ones
    index_t prev_log_idx;
    // Term of prev_log_idx entry.
    term_t prev_log_term;
    // The leader's commit_idx.
    index_t leader_commit_idx;
    // Log entries to store (empty vector for heartbeat; may send more
    // than one entry for efficiency).
    log_entry_vec entries;

    append_request copy() const {
        append_request result;
        result.current_term = current_term;
        result.prev_log_idx = prev_log_idx;
        result.prev_log_term = prev_log_term;
        result.leader_commit_idx = leader_commit_idx;
        result.entries.reserve(entries.size());
        for (const auto& e: entries) {
            // result.entries.push_back(std::make_shared<log_entry>(e));
        }
        return result;
    }
};

struct append_reply {
    struct rejected {
        index_t non_matching_idx;
        index_t last_idx;
    };
    struct accepted {
        index_t last_new_idx;
    };
    // Current term, for leader to update itself.
    term_t current_term;
    index_t commit_idx;
    std::variant<rejected, accepted> result;
};

struct vote_request {
    // The candidate’s term.
    term_t current_term;
    // The index of the candidate's last log entry.
    index_t last_log_idx;
    // The term of the candidate's last log entry.
    term_t last_log_term;
    // True if this is prevote request
    bool is_prevote;

    bool force;
};
struct timeout_now {
    // Current term on a leader
    term_t current_term;
};
struct vote_reply {
    // Current term, for the candidate to update itself.
    term_t current_term;
    // True means the candidate received a vote.
    bool vote_granted;
    // True if it is a reply to prevote request
    bool is_prevote;
};

struct install_snapshot {
    // Current term on a leader
    term_t current_term;
    // A snapshot to install
    snapshot_descriptor snp;
};

struct snapshot_reply {
    // Follower current term
    term_t current_term;
    // True if the snapshot was applied, false otherwise.
    bool success;
};
struct read_quorum {
    // The leader's term.
    term_t current_term;
    // The leader's commit_idx. Has the same semantics
    // as in append_entries.
    index_t leader_commit_idx;
    // The id of the read barrier. Only valid within this term.
    read_id id;
};

struct read_quorum_reply {
    // The leader's term, as sent in the read_quorum request.
    // read_id is only valid (and unique) within a given term.
    term_t current_term;
    // Piggy-back follower's commit_idx, for the same purposes
    // as in append_reply::commit_idx
    index_t commit_idx;
    // Copy of the id from a read_quorum request
    read_id id;
};

// using rpc_message = std::variant<append_request,
//       append_reply,
//       vote_request,
//       vote_reply,
//       snapshot_reply>;     


using rpc_message = std::variant<append_request,
      append_reply,
      vote_request,
      vote_reply,
      install_snapshot,
      snapshot_reply,
      timeout_now,
      read_quorum,
      read_quorum_reply>;

}