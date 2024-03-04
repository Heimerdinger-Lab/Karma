#pragma once
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <span>
#include <string>

#include "karma-util/coding.h"
constexpr const size_t aligned_buf_alignment = 4096;
class aligned_buf {
   public:
    aligned_buf(uint64_t wal_offset, uint64_t len = aligned_buf_alignment)
        : m_wal_offset(wal_offset), m_capacity(len) {
        m_buf = (char*)aligned_alloc(aligned_buf_alignment, m_capacity);
        memset(m_buf, 0, m_capacity);
    };
    ~aligned_buf() { free(m_buf); }
    bool write_buf(uint64_t cursor, std::span<char> data) {
        uint64_t pos = limit();
        assert((m_wal_offset + pos) == cursor);
        if (pos + data.size() > m_capacity) {
            return false;
        }
        size_t write_size = data.size();
        const char* write_data = data.data();
        std::memcpy(m_buf + pos, write_data, write_size);
        m_limit.fetch_add(write_size, std::memory_order_release);
        return true;
    };

    bool covers(uint64_t wal_offset, uint64_t len) {
        return (m_wal_offset <= wal_offset) && (wal_offset + len <= m_wal_offset + limit());
    }
    bool partial() { return limit() > 0 && limit() < m_capacity; }
    bool write_u64(uint64_t cursor, uint64_t value) {
        uint64_t pos = limit();
        if (pos + 8 > m_capacity) {
            return false;
        }
        std::string data;
        PutFixed64(&data, value);
        return write_buf(cursor, std::span<char>(data));
    };
    bool write_u32(uint64_t cursor, uint64_t value) {
        uint64_t pos = limit();
        if (pos + 4 > m_capacity) {
            return false;
        }
        std::string data;
        PutFixed64(&data, value);
        return write_buf(cursor, std::span<char>(data));
    };
    uint32_t read_u32(uint64_t cursor) { return 0; };
    uint64_t read_u64(uint64_t cursor) { return 0; }
    uint64_t limit() { return m_limit.load(std::memory_order_relaxed); }
    uint64_t remaining() { return m_capacity - limit(); }
    uint64_t wal_offset() { return m_wal_offset; }
    char* buf() { return m_buf; }
    uint64_t capacity() { return m_capacity; }

   private:
    // [m_wal_offset, m_wal_offset + m_capacity)
    uint64_t m_wal_offset = 0;
    uint64_t m_capacity = 0;
    std::atomic_uint64_t m_limit{0};
    char* m_buf;
};