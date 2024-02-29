#pragma once
#include <cstdint>
#include <iostream>
#include <map>
class write_window {
   public:
    write_window(uint64_t commit_offset = 0) : m_commit_offset(commit_offset) {}
    void commit(uint64_t wal_offset, uint64_t len) {
        // std::cout << "commit: " << wal_offset << ", len = " << len << std::endl;
        m_committed[wal_offset] = len;
        advance();
    };
    void advance() {
        for (auto it = m_committed.begin(); it != m_committed.end();) {
            if (m_commit_offset < it->first) {
                break;
            }
            m_commit_offset = std::max(it->first + it->second, m_commit_offset);
            it = m_committed.erase(it);
        }
        // std::cout << "end of advance" << std::endl;
    }
    uint64_t commit_offset() { return m_commit_offset; }

   private:
    uint64_t m_commit_offset;
    std::map<uint64_t, uint64_t> m_committed;
};