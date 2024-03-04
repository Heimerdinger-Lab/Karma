#pragma once
#include <unistd.h>

#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

#include "karma-store/buf/aligned_buf.h"
#include "karma-util/coding.h"
class aligned_buf_writer {
   public:
    aligned_buf_writer(uint64_t cursor) : m_cursor(cursor) {
        uint64_t from = cursor / aligned_buf_alignment * aligned_buf_alignment;
        m_current = std::make_unique<aligned_buf>(from);
    };
    ~aligned_buf_writer() {}

    uint64_t cursor() { return m_cursor; }
    bool write(std::span<char> data) {
        uint64_t len = data.size();
        uint64_t pos = 0;
        while (true) {
            uint64_t r = m_current->remaining();
            if (r >= (len - pos)) {
                m_current->write_buf(m_cursor + pos, data);
                pos += data.size();
                break;
            } else {
                // 前r个
                m_current->write_buf(m_cursor + pos, data.subspan(0, r));
                data = data.subspan(r);
                pos += r;
                // 写满了
                m_full.push_back(m_current);
                m_current =
                    std::make_shared<aligned_buf>(m_current->wal_offset() + m_current->limit());
            }
        };
        m_cursor += len;
        m_dirty = true;
        return true;
    };
    bool write_u32(uint32_t value) {
        std::string str;
        PutFixed32(&str, value);
        assert(str.size() == 4);
        return write(std::span<char>(str));
    }

    bool write_u64(uint64_t value) {
        std::string str;
        PutFixed64(&str, value);
        return write(std::span<char>(str));
    }
    bool dirty() { return m_dirty; }
    void set_dirty(bool value) { m_dirty = value; }

   public:
    uint64_t m_cursor;
    std::vector<std::shared_ptr<aligned_buf>> m_full;
    std::shared_ptr<aligned_buf> m_current;
    bool m_dirty = true;
};