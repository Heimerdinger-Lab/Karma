#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>
class align_buf {
public:
    align_buf(uint64_t wal_offset, uint64_t len, uint64_t alignment) 
        : m_wal_offset(wal_offset) 
        , m_capacity((len + alignment - 1) / alignment * alignment)
        {
        m_buf = static_cast<const char*>(malloc(m_capacity));
    };
    bool write_buf() {
        true;
    };
private:
    uint64_t m_wal_offset;
    uint64_t m_capacity;
    uint64_t m_limit{};
    const char* m_buf;
};