#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <variant>
#include <memory>
namespace raft {
using term_t = uint64_t;
using index_t = uint64_t;
using server_id = uint64_t;
using command_t = std::string;
using command_cref = std::reference_wrapper<const command_t>;
using group_id = uint64_t;

struct log_entry {
    // Dummy entry is used when a leader needs to commit an entry
    // (after leadership change for instance) but there is nothing
    // else to commit.
    struct dummy {};
    term_t term;
    index_t idx;
    std::variant<command_t, dummy> data;
};
using log_entry_ptr = std::shared_ptr<log_entry>;
using log_entry_vec = std::vector<log_entry_ptr>;

using server_address_set = std::unordered_set<server_id>;
using config_member_set = std::unordered_set<server_id>;

struct configuration {
    config_member_set m_current;
    config_member_set m_previous;
};

}