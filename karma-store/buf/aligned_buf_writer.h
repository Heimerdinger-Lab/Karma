#pragma once
#include "karma-store/buf/aligned_buf.h"
#include "karma-util/coding.h"
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <vector>
class aligned_buf_writer {
public:
    aligned_buf_writer(uint64_t cursor)
        : m_cursor(cursor) {
        uint64_t from = cursor / aligned_buf_alignment * aligned_buf_alignment;
        m_current = std::make_shared<aligned_buf>(from);
        m_full = std::make_shared<std::vector<std::shared_ptr<aligned_buf>>>();
    };
    ~aligned_buf_writer() {
        std::cout << "~abf" << std::endl;
    }
    // bool buffering() {
    //     return m_buffering;
    // }
    uint64_t cursor() {
        return m_cursor;
    }
    bool write(sslice data) {
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
                // data.remove_prefix(0);
                m_current->write_buf(m_cursor + pos, sslice(data.data(), r));
                data.remove_prefix(r);
                pos += r;
                // 写满了
                m_full->push_back(m_current);
                std::cout << "new: " << m_current->wal_offset() + m_current->limit() << std::endl;
                m_current = std::make_shared<aligned_buf>(m_current->wal_offset() + m_current->limit());        
            }
        };
        std::cout << "len = " << len << std::endl;
        m_cursor += len;
        // m_buffering = true;
        m_dirty = true;
        return true;
    };
    bool write_u32(uint32_t value) {
        std::string str;
        PutFixed32(&str, value);
        assert(str.size() == 4);
        sslice sstr(str);
        return write(sstr);
    }

    bool write_u64(uint64_t value) {
        std::string str;
        PutFixed64(&str, value);
        sslice sstr(str);
        return write(sstr);
    }
    bool dirty() {
        return m_dirty;
        // return true;
    }
    void set_dirty(bool value) {
        m_dirty = value;
    }
public:
    uint64_t m_cursor;
    std::shared_ptr<std::vector<std::shared_ptr<aligned_buf>>> m_full;
    std::shared_ptr<aligned_buf> m_current;
    bool m_dirty = true;
};