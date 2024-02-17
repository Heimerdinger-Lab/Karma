#pragma once
#include "karma-store/buf/aligned_buf.h"
#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-store/buf/aligned_buf_reader.h"
#include "karma-util/crc32c.h"
#include "karma-util/sslice.h"
#include <cstdint>
#include <fcntl.h>
#include <future>
#include <memory>
#include <string>
#include <iostream>
#include <unistd.h>

class segment_file {
public:
    segment_file(uint64_t wal_offset, uint64_t size, std::string path) 
        : m_wal_offset(wal_offset)
        , m_size(size) 
        , m_path(path)
        , m_status(status::closed)
        , m_written(0) {
        // std::cout << "wal_offset = " << wal_offset << std::endl;
        // std::cout << "m_size = " << m_size << std::endl;
        // std::cout << "m_path = " << m_path << std::endl;
    }
    uint64_t wal_offset() {
        return m_wal_offset;
    }
    uint64_t size() {
        return m_size;
    }
    void set_read_status() {
        m_status = status::read;
    }
    void set_read_write_status() {
        m_status = status::read_write;
    }
    void set_written(uint64_t written) {
        m_written = written;
    }
    void read_exact_at(sslice* data, uint32_t size, uint64_t wal_offset) {
        auto buf = aligned_buf_reader::alloc_read_buf(wal_offset, size);
        // int ret = ::read(m_fd, buf->buf(), buf->capacity());
        int ret = ::pread(m_fd, buf->buf(), buf->capacity(), buf->wal_offset() - m_wal_offset);
        std::cout << "read ret = " << ret << std::endl;
        ::memcpy((char *)data->data(), buf->buf() + wal_offset - buf->wal_offset(), size);
    }
    uint32_t cal_crc32(sslice &slice) {
        return 0;
    }
    uint32_t cal_length_type(uint32_t size, uint8_t type) {
        return size << 8 | type;
    }
    uint64_t append_record(std::shared_ptr<aligned_buf_writer> writer, sslice& slice) {
        // auto crc32 = cal_crc32(slice);
        uint32_t crc32 = crc32c::Value(slice.data(), slice.size());
        uint32_t length_type = cal_length_type(slice.size(), 0);
        std::cout << "crc32 = " << crc32 << std::endl;
        std::cout << "length_type = " << length_type << std::endl;
        writer->write_u32(crc32);
        writer->write_u32(length_type);
        writer->write(slice);
        m_written += 4 + 4 + slice.size();
        return true;
    };
    void append_footer(std::shared_ptr<aligned_buf_writer> writer) {
        if (m_size - m_written < 8) {
            // 直接全填0
            // 直接略过了
            m_written = m_size;
            return;
        }


        uint32_t padding_length = m_size - m_written - 4 - 4;
        // 有 padding_length 个0
        sslice s(0);
        // auto crc32 = cal_crc32(sslice(padding_length))
        auto length_type = cal_length_type(padding_length, 1);
        writer->write_u32(0);
        writer->write_u32(length_type);
        writer->write(s);
    }
    bool open_and_create() {
        int fd = ::open(m_path.c_str(), O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
        int ret = ::fallocate(fd, 0, 0, 1048576);
        if (ret != 0) {
            return false;
        }
        ret = ::fsync(fd);
        if (ret != 0) {
            return false;
        }
        std::cout << "wal_offset = " << m_wal_offset << std::endl;
        if (fd < 0) {
            return false;
        }
        m_fd = fd;
        m_status = status::read_write;
        return true;
    }
    static void alloc_segment();
    bool can_hold(uint64_t size) {
        return m_status == status::read_write && m_written + 8 + size <= m_size;
    }
    int fd() {
        return m_fd;
    }
private:
    enum status {
        closed,
        read_write,
        read,
    };
    uint64_t m_wal_offset;
    uint64_t m_size;
    uint64_t m_written;
    int m_fd;
    std::string m_path;
    status m_status;
};