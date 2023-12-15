#pragma once 
#include <optional>

#include "karma-raft/common.h"
namespace raft {
// The value of the first index is 1.
class log {
    snapshot_descriptor m_snapshot;
    log_entry_vec m_log;
    // 单个common的大小限制
    size_t m_max_command_size;
    index_t m_first_idx;
    index_t m_stable_idx = index_t(0);
    index_t m_last_conf_idx = index_t(0);
    index_t m_prev_conf_idx = index_t(0);
    // 总体的内存使用量
    size_t m_memory_usage;
private:
    void init_last_conf_idx();
    void truncate_uncommitted(index_t idx);
    log_entry_ptr& get_entry(index_t idx);
    const log_entry_ptr& get_entry(index_t idx) const;
    size_t range_memory_usage(log_entry_vec::iterator first, log_entry_vec::iterator last) const;
public:
    explicit log(snapshot_descriptor snp, log_entry_vec log = {}, size_t max_common_size = sizeof(log_entry)) 
        : m_snapshot(std::move(snp))
        , m_log(std::move(log)) {
        if (m_log.empty()) {
            m_first_idx = m_snapshot.idx + 1;
        } else {
            m_first_idx = m_log[0]->idx;
        }   
        m_memory_usage = range_memory_usage(m_log.begin(), m_log.end());
        stable_to(last_idx());
        init_last_conf_idx();
    }
    log_entry_ptr& operator[] (size_t i);
    void emplace_back(log_entry_ptr&& e);
    void stable_to(index_t idx);
    bool empty() const;
    bool is_up_to_date(index_t idx, term_t term) const;
    index_t next_idx() const;
    index_t last_idx() const;
    index_t last_conf_idx() const;
    index_t stable_idx() const;
    term_t last_term() const;
    size_t in_memory_size() const; 
    std::pair<bool, term_t> match_term(index_t idx, term_t term) const;
    std::optional<term_t> term_for(index_t idx) const;
    const configuration& get_configuration() const;

    // Return the last configuration entry with index smaller than or equal to `idx`.
    // Precondition: `last_idx()` >= `idx` >= `get_snapshot().idx`;
    // there is no way in general to learn configurations before the last snapshot.
    const configuration& last_conf_for(index_t idx) const;

    // Return the previous configuration, if available (otherwise return nullptr).
    // The returned pointer, if not null, is only valid until the next operation on the log.
    const configuration* get_prev_configuration() const;

    index_t maybe_append(log_entry_vec&& entries);
    void apply_snapshot(snapshot_descriptor&& snp, size_t max_trailing_entries, size_t max_trailing_bytes);
    const snapshot_descriptor& get_snapshot() const {
        return m_snapshot;
    }
    template <typename T>
    requires std::is_same_v<T, log_entry> ||
             std::is_same_v<T, command_t> || std::is_same_v<T, configuration> || std::is_same_v<T, log_entry::dummy>
    static inline size_t memory_usage_of(const T& v, size_t max_command_size) {
        if constexpr(std::is_same_v<T, command_t>) {
            // We account for sizeof(log_entry) for "small" commands,
            // since the overhead of log_entries can take up significant memory.
            return max_command_size > sizeof(log_entry) && v.size() < max_command_size - sizeof(log_entry)
                ? v.size() + sizeof(log_entry)
                : v.size();
        }
        if constexpr(std::is_same_v<T, log_entry>) {
            if (const auto* c = get_if<command_t>(&v.data); c != nullptr) {
                return memory_usage_of(*c, max_command_size);
            }
        }
        return 0;
    }
};
}