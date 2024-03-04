#pragma once
#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <span>
#include <string>

#include "karma-store/buf/aligned_buf.h"
#include "karma-store/buf/aligned_buf_reader.h"
#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-util/crc32c.h"
#include "karma-util/sslice.h"

class segment_file {
   public:
    segment_file(uint64_t wal_offset, uint64_t size, std::string path)
        : m_wal_offset(wal_offset),
          m_size(size),
          m_path(path),
          m_status(status::closed),
          m_written(0) {}
    uint64_t wal_offset() { return m_wal_offset; }
    uint64_t size() { return m_size; }
    void set_read_status() { m_status = status::read; }
    bool read_write_status() { return m_status == status::read_write; }
    void set_read_write_status() { m_status = status::read_write; }
    void set_written(uint64_t written) { m_written = written; }
    bool read_exact_at(sslice *data, uint64_t wal_offset, uint32_t size);
    uint32_t cal_crc32(sslice &slice) { return 0; }
    uint32_t cal_length_type(uint32_t size, uint8_t type = 0) { return size << 8 | type; }
    uint64_t append_record(aligned_buf_writer &writer, std::span<char> slice);
    void append_footer(aligned_buf_writer &writer);
    bool open_and_create(uint32_t segment_file_size);
    static void alloc_segment();
    bool can_hold(uint64_t size);
    int fd() { return m_fd; }

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