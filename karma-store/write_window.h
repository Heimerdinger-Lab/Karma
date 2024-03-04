#pragma once
#include <cstdint>
#include <map>
class write_window {
   public:
    write_window(uint64_t commit_offset = 0) : m_committed_offset(commit_offset) {}
    void commit(uint64_t wal_offset, uint64_t len);
    uint64_t current_committed_offset() { return m_committed_offset; }

   private:
    void advance();
    uint64_t m_committed_offset;
    std::map<uint64_t, uint64_t> m_committed;
};