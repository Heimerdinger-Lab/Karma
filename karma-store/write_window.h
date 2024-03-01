#pragma once
#include <cstdint>
#include <iostream>
#include <map>
class write_window {
   public:
    write_window(uint64_t commit_offset = 0) : m_commit_offset(commit_offset) {}
    void commit(uint64_t wal_offset, uint64_t len);
    void advance();
    uint64_t commit_offset() { return m_commit_offset; }

   private:
    uint64_t m_commit_offset;
    std::map<uint64_t, uint64_t> m_committed;
};