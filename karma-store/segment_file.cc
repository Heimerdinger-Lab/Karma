#include "segment_file.h"

#include <boost/log/trivial.hpp>

#include "karma-store/common.h"
bool segment_file::read_exact_at(sslice *data, uint64_t wal_offset, uint32_t size) {
    if (size <= 0) return false;
    auto buf = aligned_buf_reader::alloc_read_buf(wal_offset, size);
    int ret = ::pread(m_fd, buf->buf(), buf->capacity(), buf->wal_offset() - m_wal_offset);
    if (ret < 0) {
        BOOST_LOG_TRIVIAL(error) << "Fail to pread current segment file";
        return false;
    }
    ::memcpy((char *)data->data(), buf->buf() + wal_offset - buf->wal_offset(), size);
    return true;
}

uint64_t segment_file::append_record(aligned_buf_writer &writer, std::span<char> slice) {
    uint32_t crc32 = crc32c::Value(slice.data(), slice.size());
    uint32_t length_type = cal_length_type(slice.size());
    int size = slice.size();
    writer.write_u32(crc32);
    writer.write_u32(length_type);
    writer.write(slice);
    m_written += store::RECORD_HEADER_LENGTH + size;
    assert(m_wal_offset + m_written == writer.cursor());
    return m_written + m_wal_offset;
};

void segment_file::append_footer(aligned_buf_writer &writer) {
    if (m_size - m_written < 8) {
        uint64_t len = m_size - m_written;
        auto footer = std::string(len, '0');
        writer.write(footer);
        m_written += len;
        return;
    }
    uint32_t padding_length = m_size - m_written - store::RECORD_HEADER_LENGTH;
    auto footer = std::string(padding_length, '0');
    auto length_type = cal_length_type(padding_length, 1);
    writer.write_u32(0);
    writer.write_u32(length_type);
    writer.write(footer);
    m_written = m_size;
    assert(m_wal_offset + m_written == writer.cursor());
}

bool segment_file::open_and_create(uint32_t segment_file_size) {
    BOOST_LOG_TRIVIAL(trace) << "Try to open or create current file: " << m_path;
    int fd = ::open(m_path.c_str(), O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        BOOST_LOG_TRIVIAL(error) << "Fail to open this segment file: " << m_path;
        return false;
    }
    int ret = ::fallocate(fd, 0, 0, segment_file_size);
    if (ret != 0) {
        BOOST_LOG_TRIVIAL(error) << "Fail to fallocate to " << segment_file_size;
        return false;
    }
    ret = ::fsync(fd);
    if (ret != 0) {
        BOOST_LOG_TRIVIAL(error) << "Fail to fsync meta data";
        return false;
    }

    m_fd = fd;
    m_status = status::read_write;
    return true;
}

bool segment_file::can_hold(uint64_t size) {
    return (m_status == status::read_write) &&
           (m_written + store::RECORD_HEADER_LENGTH + size <= m_size);
}
