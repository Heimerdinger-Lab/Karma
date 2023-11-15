#include "log.h"
namespace raft {
log_entry_ptr &log::operator[](size_t i)
{
    return get_entry(index_t(i));
}

void log::emplace_back(log_entry_ptr &&e)
{
    m_log.emplace_back(std::move(e));
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

index_t log::stable_idx() const
{
    return m_stable_idx;
}

term_t log::last_term() const
{
    return m_log.back()->term;
}
/*
    看第index项的term是不是传入的term
*/
std::pair<bool, term_t> log::match_term(index_t idx, term_t term) const
{
    int i = idx - m_first_idx;
    if (i >= m_log.size()) {
        return std::make_pair(false, term_t(0));        
    }
    auto my_term = m_log[i]->term;
    return my_term == term ? std::make_pair(true, term_t(0)) : std::make_pair(false, my_term);
}

std::optional<term_t> log::term_for(index_t idx) const
{
    if (!m_log.empty() && idx >= m_first_idx) {
        return m_log[idx - m_first_idx]->term;
    }
    return {};
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
/*
    删掉逻辑idx之后的项目
*/
void log::truncate_uncommitted(index_t idx)
{
    auto it = m_log.begin() + (idx - m_first_idx);
    m_log.erase(it, m_log.end());
    // 持久化了并不代表它commit了，持久化是commit的必要条件
    // 所以stable了的数据也可以修改。
    stable_to(std::min(m_stable_idx, last_idx()));
}

log_entry_ptr &log::get_entry(index_t idx)
{
    return m_log[idx - m_first_idx];
}

const log_entry_ptr &log::get_entry(index_t idx) const
{
    return m_log[idx - m_first_idx];
}
}