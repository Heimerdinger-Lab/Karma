#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <variant>
#include <memory>

using term_t = uint64_t;
using index_t = uint64_t;
using server_id = uint64_t;
using command_t = std::string;
using command_cref = std::reference_wrapper<const command_t>;


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


// class votes {
//     server_address_set _voters;
//     election_tracker _current;
//     std::optional<election_tracker> _previous;
// public:
//     votes(configuration configuration);

//     // A server is a member of this set iff
//     // it is a voter in the current or previous configuration.
//     const server_address_set& voters() const {
//         return _voters;
//     }

//     void register_vote(server_id from, bool granted);
//     vote_result tally_votes() const;

//     friend std::ostream& operator<<(std::ostream& os, const votes& v);
// };