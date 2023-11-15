#pragma once 
#include <optional>

#include "common.h"
namespace raft {
// The value of the first index is 1.
class log {
    log_entry_vec m_log;
    index_t m_first_idx;
    index_t m_stable_idx = index_t(0);
private:
    void truncate_uncommitted(index_t idx);
    log_entry_ptr& get_entry(index_t idx);
    const log_entry_ptr& get_entry(index_t idx) const;
public:
    explicit log(log_entry_vec log = {}) 
        : m_log(std::move(log)) {
        if (m_log.empty()) {
            m_first_idx = 1;
        } else {
            m_first_idx = m_log[0]->idx;
        }
        
    }
    log_entry_ptr& operator[] (size_t i);
    void emplace_back(log_entry_ptr&& e);
    void stable_to(index_t idx);
    bool empty() const;
    bool is_up_to_date(index_t idx, term_t term) const;
    index_t next_idx() const;
    index_t last_idx() const;
    index_t stable_idx() const;
    term_t last_term() const;
    std::pair<bool, term_t> match_term(index_t idx, term_t term) const;
    std::optional<term_t> term_for(index_t idx) const;
    index_t maybe_append(log_entry_vec&& entries);
};
}