#pragma once
#include "karma-store/buf/aligned_buf.h"
#include "karma-util/coding.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <unistd.h>
#include <vector>
class aligned_buf_writer {
public:
    aligned_buf_writer(uint64_t cursor)
        : m_cursor(cursor) {
        m_current = std::make_shared<aligned_buf>(cursor);
    };
    bool buffering() {
        return m_buffering;
    }
    uint64_t cursor() {
        return m_cursor;
    }
    bool write(sslice& data) {
        uint64_t len = data.size();
        uint64_t pos = 0;
        while (true) {
            uint64_t r = m_current->remaining();
            if (r >= len - pos) {
                m_current->write_buf(m_cursor + pos, data);
                pos += data.size();
                break;
            } else {
                // 前r个
                data.remove_prefix(0);
                m_current->write_buf(m_cursor + pos, sslice(data.data(), r));
                data.remove_prefix(r);
                pos += r;
                // 写满了
                m_full.push_back(m_current);
                m_current = std::make_shared<aligned_buf>(m_current->wal_offset() + m_current->limit());        
            }
        };
        m_cursor += len;
        m_buffering = true;
        return true;
    };
    bool write_u32(uint32_t value) {
        std::string str;
        PutFixed32(&str, value);
        sslice sstr(str);
        return write(sstr);
    }
    bool write_u64(uint64_t value) {
        std::string str;
        PutFixed64(&str, value);
        sslice sstr(str);
        return write(sstr);
    }
    
public:
    uint64_t m_cursor;
    std::vector<std::shared_ptr<aligned_buf>> m_full;
    std::shared_ptr<aligned_buf> m_current;
    bool m_buffering;
};