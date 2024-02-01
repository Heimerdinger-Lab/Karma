#include "karma-raft/log.h"
#include "common.h"
#include <utility>
#include <variant>
namespace raft {
log_entry_ptr &log::operator[](size_t i)
{
    return get_entry(index_t(i));
}

void log::emplace_back(log_entry_ptr &&e)
{
    m_log.emplace_back(std::move(e));
    m_memory_usage += memory_usage_of(*m_log.back(), m_max_command_size);
    if (std::holds_alternative<configuration>(m_log.back()->data)) {
        m_prev_conf_idx = m_last_conf_idx;
        m_last_conf_idx = last_idx();
    }
}

void log::stable_to(index_t idx)
{
    m_stable_idx = idx;
}

bool log::empty() const
{
    return m_log.empty();
}

bool log::is_up_to_date(index_t idx, term_t term) const
{
    return term > last_term() || (term == last_term() && idx >= last_idx());
}

index_t log::next_idx() const
{
    return last_idx() + index_t(1);
}

index_t log::last_idx() const
{
    return m_first_idx + (index_t(m_log.size()) - index_t(1));
}

index_t log::last_conf_idx() const {
    return m_last_conf_idx ? m_last_conf_idx : m_snapshot.idx;
}

index_t log::stable_idx() const
{
    return m_stable_idx;
}

term_t log::last_term() const
{
    if (m_log.empty()) {
        return m_snapshot.term;
    }
    return m_log.back()->term;
}
size_t log::in_memory_size() const {
    return m_log.size();
}
/*
    看第index项的term是不是传入的term
*/
std::pair<bool, term_t> log::match_term(index_t idx, term_t term) const
{
    if (idx == 0) {
        return std::make_pair(true, term_t(0));
    }

    // We got an AppendEntries inside out snapshot, it has to much by
    // log matching property
    if (idx < m_snapshot.idx) {
        return std::make_pair(true, last_term());
    }

    term_t my_term;

    if (idx == m_snapshot.idx) {
        my_term = m_snapshot.term;
    } else {
        auto i = idx - m_first_idx;

        if (i >= m_log.size()) {
            // We have a gap between the follower and the leader.
            return std::make_pair(false, term_t(0));
        }

        my_term =  m_log[i]->term;
    }

    return my_term == term ? std::make_pair(true, term_t(0)) : std::make_pair(false, my_term);
}

std::optional<term_t> log::term_for(index_t idx) const
{
    if (!m_log.empty() && idx >= m_first_idx) {
        return m_log[idx - m_first_idx]->term;
    }
    if (idx == m_snapshot.idx) {
        return m_snapshot.term;
    }
    return {};
}
const configuration& log::get_configuration() const {
    return m_last_conf_idx ? std::get<configuration>(m_log[m_last_conf_idx - m_first_idx]->data) : m_snapshot.config;
}
const configuration& log::last_conf_for(index_t idx) const {
    assert(last_idx() >= idx);
    assert(idx >= m_snapshot.idx);

    if (!m_last_conf_idx) {
        assert(!m_prev_conf_idx);
        return m_snapshot.config;
    }

    if (idx >= m_last_conf_idx) {
        return std::get<configuration>(get_entry(m_last_conf_idx)->data);
    }

    if (!m_prev_conf_idx) {
        // There are no config entries between _snapshot and _last_conf_idx.
        return m_snapshot.config;
    }

    if (idx >= m_prev_conf_idx) {
        return std::get<configuration>(get_entry(m_prev_conf_idx)->data);
    }

    for (; idx > m_snapshot.idx; --idx) {
        if (auto cfg = std::get_if<configuration>(&get_entry(idx)->data)) {
            return *cfg;
        }
    }

    return m_snapshot.config;
}
const configuration* log::get_prev_configuration() const {
    if (m_prev_conf_idx) {
        return &std::get<configuration>(get_entry(m_prev_conf_idx)->data);
    }

    if (m_last_conf_idx > m_snapshot.idx) {
        return &m_snapshot.config;
    }

    // _last_conf_idx <= _snapshot.idx means we only have the last configuration (from the snapshot).
    return nullptr;
}
/*
    将entries 添加到本地log entries后面
    如果本地log entries存在不符合的项，trim掉后面的，以传入的entris为准
*/
index_t log::maybe_append(log_entry_vec &&entries)
{
    index_t last_new_idx = entries.back()->idx;
    for (auto& e: entries) {
        if (e->idx <= last_idx()) {
            if (e->idx < m_first_idx) {
                continue;
            }
            if (e->term == get_entry(e->idx)->term) {
                continue;;
            }
            truncate_uncommitted(e->idx);
        }
        emplace_back(std::move(e));
    }
    return last_new_idx;
}
void log::apply_snapshot(snapshot_descriptor&& snp, size_t max_trailing_entries, size_t max_trailing_bytes) {
    // 传入一个snapshot
    size_t released_memory;
    auto idx = snp.idx;
    if (idx > last_idx()) {
        // 说明当前log中的entry都落后了。
        released_memory = std::exchange(m_memory_usage, 0);
        m_log.clear();
        m_log.shrink_to_fit();
        m_first_idx = idx + 1;
    } else {
        auto entries_to_remove = m_log.size() - (last_idx() - idx);
        // entries_to_remove = std::max((unsigned long)0, entries_to_remove - max_trailing_entries);
        size_t trailing_bytes = 0;
        for (size_t i = 0; i < max_trailing_entries && entries_to_remove > 0; ++i) {
            trailing_bytes += memory_usage_of(* m_log[entries_to_remove - 1], m_max_command_size);
            if (trailing_bytes > max_trailing_bytes) {
                break;
            }
            --entries_to_remove;
        }
        released_memory = range_memory_usage(m_log.begin(), m_log.begin() + entries_to_remove);
        m_log.erase(m_log.begin(), m_log.begin() + entries_to_remove);
        m_log.shrink_to_fit();
        m_memory_usage -= released_memory;
        m_first_idx = m_first_idx + entries_to_remove;
    }
    m_stable_idx = std::max(idx, m_stable_idx);
    if (idx >= m_prev_conf_idx) {
        m_prev_conf_idx = 0;
        if (idx >= m_last_conf_idx) {
            m_last_conf_idx = 0;
        }
    }
    m_snapshot = std::move(snp);
}
void log::init_last_conf_idx() {
    for (auto it = m_log.rbegin(); it != m_log.rend() && (**it).idx != m_snapshot.idx; ++it) {
        if (std::holds_alternative<configuration>((**it).data)) {
            if (m_last_conf_idx == index_t{0}) {
                m_last_conf_idx = (**it).idx;
            } else {
                m_prev_conf_idx = (**it).idx;
                break;
            }
        }
   }
}
/*
    删掉逻辑idx之后的项目
*/
void log::truncate_uncommitted(index_t idx)
{
    auto it = m_log.begin() + (idx - m_first_idx);
    const auto released_memory = range_memory_usage(it, m_log.end());
    m_log.erase(it, m_log.end());
    m_log.shrink_to_fit();
    m_memory_usage -= released_memory;
    // 持久化了并不代表它commit了，持久化是commit的必要条件
    // 所以stable了的数据也可以修改。
    stable_to(std::min(m_stable_idx, last_idx()));
    if (m_last_conf_idx > last_idx()) {
        m_last_conf_idx = m_prev_conf_idx;
        m_prev_conf_idx = index_t(0);
    }
}

log_entry_ptr &log::get_entry(index_t idx)
{
    return m_log[idx - m_first_idx];
}

const log_entry_ptr &log::get_entry(index_t idx) const
{
    return m_log[idx - m_first_idx];
}

size_t log::range_memory_usage(log_entry_vec::iterator first, log_entry_vec::iterator last) const {
    size_t result = 0;
    for (auto it = first; it != last; ++it) {
        result += memory_usage_of(**it, m_max_command_size);
    }
    return result;
}
}