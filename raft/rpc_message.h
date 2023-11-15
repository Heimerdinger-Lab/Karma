#pragma once
#include "common.h"

namespace raft {
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
            result.entries.push_back(std::make_shared<log_entry>(e));
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
};



using rpc_message = std::variant<append_request,
      append_reply,
      vote_request,
      vote_reply>;
}