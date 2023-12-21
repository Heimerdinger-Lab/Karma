#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fmt/format.h>
#include <span>
#include <stdint.h>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_set>
namespace raft {
using term_t = uint64_t;
using index_t = uint64_t;
using read_id = uint64_t;
using server_id = uint64_t;
using snapshot_id = uint64_t;
using command_t = std::string;
using command_cref = std::reference_wrapper<const command_t>;
using group_id = uint64_t;


struct server_address {
    server_id id;
    std::string host;
    uint16_t port;
    server_address(server_id id, std::string host = "", uint16_t port = 0)
        : id(std::move(id)), host(std::move(host)), port(port) {
    }

    bool operator==(const server_address& rhs) const {
        return id == rhs.id;
    }

    bool operator==(const raft::server_id& rhs) const {
        return id == rhs;
    }

    bool operator<(const server_address& rhs) const {
        return id < rhs.id;
    }
    std::string encode() {
        // id:host:port
    }
    static server_address parse(std::span<char>);
};
struct config_member {
    server_address addr;
    bool can_vote;

    config_member(server_address addr, bool can_vote = false)
        : addr(std::move(addr)), can_vote(can_vote) {
    }

    bool operator==(const config_member& rhs) const {
        return addr == rhs.addr;
    }

    bool operator==(const raft::server_id& rhs) const {
        return addr.id == rhs;
    }

    bool operator<(const config_member& rhs) const {
        return addr < rhs.addr;
    }
};
struct server_address_hash {
    size_t operator()(const raft::server_address& address) const {
        return std::hash<raft::server_id>{}(address.id);
    }
};

struct config_member_hash {
    size_t operator()(const raft::config_member& s) const {
        return std::hash<raft::server_id>{}(s.addr.id);
    }
};

using server_address_set = std::unordered_set<server_address, server_address_hash>;
using config_member_set = std::unordered_set<config_member, config_member_hash>;

struct configuration {
    config_member_set m_current;
    config_member_set m_previous;
    explicit configuration(config_member_set current_arg = {}, config_member_set previous_arg = {}) 
        : m_current(current_arg)
        , m_previous(previous_arg){
    }
    bool is_joint() const {
        return !m_previous.empty();
    }
    static size_t voter_count(const config_member_set& c_new) {
        return std::count_if(c_new.begin(), c_new.end(), [] (const config_member& s) { return s.can_vote; });
    }
    // Enter a joint configuration given a new set of servers.
    void enter_joint(config_member_set c_new) {
        // if (c_new.empty()) {
        //     throw std::invalid_argument("Attempt to transition to an empty Raft configuration");
        // }
        m_previous = std::move(m_current);
        m_current = std::move(c_new);
    }
    void leave_joint() {
        assert(is_joint());
        m_previous.clear();
    }
    // Same as contains() but true only if the member can vote.
    bool can_vote(server_id id) const {
        bool can_vote = false;
        auto it = m_current.find(server_address(id));
        if (it != m_current.end()) {
            can_vote |= it->can_vote;
        }

        it = m_previous.find(server_address(id));
        if (it != m_previous.end()) {
            can_vote |= it->can_vote;
        }

        return can_vote;
    }
};

struct snapshot_descriptor {
    // Index and term of last entry in the snapshot
    index_t idx = index_t(0);
    term_t term = term_t(0);
    // The committed configuration in the snapshot
    configuration config;
    // Id of the snapshot.
    snapshot_id id;
};



struct log_entry {
    // Dummy entry is used when a leader needs to commit an entry
    // (after leadership change for instance) but there is nothing
    // else to commit.
    struct dummy {};
    term_t term;
    index_t idx;
    std::variant<command_t, configuration, dummy> data;
};
using log_entry_ptr = std::shared_ptr<log_entry>;
using log_entry_vec = std::vector<log_entry_ptr>;
class failure_detector {
public:
    virtual ~failure_detector() {}
    virtual bool is_alive(server_id server) = 0;
};


struct error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct not_a_leader : public error {
    server_id leader;
    explicit not_a_leader(server_id l) : error(fmt::format("Not a leader, leader: {}", l)), leader(l) {}
};

struct not_a_member : public error {
    explicit not_a_member(std::string err) : error(std::move(err)) {}
};


struct dropped_entry : public error {
    dropped_entry() : error("Entry was dropped because of a leader change") {}
};

struct commit_status_unknown : public error {
    commit_status_unknown() : error("Commit status of the entry is unknown") {}
};

struct stopped_error : public error {
    explicit stopped_error(const std::string& reason = "")
            : error(!reason.empty()
                    ? fmt::format("Raft instance is stopped, reason: \"{}\"", reason)
                    : std::string("Raft instance is stopped")) {}
};

struct conf_change_in_progress : public error {
    conf_change_in_progress() : error("A configuration change is already in progress") {}
};

struct config_error : public error {
    using error::error;
};


struct timeout_error : public error {
    using error::error;
};

struct state_machine_error: public error {
    state_machine_error()
        : error(fmt::format("State machine error.")) {}
};

// Should be thrown by the rpc implementation to signal that the connection to the peer has been lost.
// It's unspecified if any actions caused by rpc were actually performed on the target node.
struct transport_error: public error {
    using error::error;
};
 

struct command_is_too_big_error: public error {
    size_t command_size;
    size_t limit;

    command_is_too_big_error(size_t command_size, size_t limit)
        : error(fmt::format("Command size {} is greater than the configured limit {}", command_size, limit))
        , command_size(command_size)
        , limit(limit) {}
};

struct no_other_voting_member : public error {
    no_other_voting_member() : error("Cannot stepdown because there is no other voting member") {}
};

struct request_aborted : public error {
    request_aborted() : error("Request is aborted by a caller") {}
};


}

