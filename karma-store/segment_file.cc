#include "segment_file.h"
void segment_file::read_exact_at(sslice *data, uint32_t size, uint64_t wal_offset) {
    if (size <= 0) return;
    auto buf = aligned_buf_reader::alloc_read_buf(wal_offset, size);
    // int ret = ::read(m_fd, buf->buf(), buf->capacity());
    int ret = ::pread(m_fd, buf->buf(), buf->capacity(), buf->wal_offset() - m_wal_offset);
    std::cout << "read ret = " << ret << std::endl;
    ::memcpy((char *)data->data(), buf->buf() + wal_offset - buf->wal_offset(), size);
}

uint64_t segment_file::append_record(aligned_buf_writer &writer, sslice slice) {
    // auto crc32 = cal_crc32(slice);
    uint32_t crc32 = crc32c::Value(slice.data(), slice.size());
    uint32_t length_type = cal_length_type(slice.size(), 0);
    int len = slice.size();
    // std::cout << "crc32 = " << crc32 << std::endl;
    // std::cout << "length_type = " << length_type << std::endl;
    writer.write_u32(crc32);
    writer.write_u32(length_type);
    writer.write(slice);
    m_written += 4 + 4 + len;
    assert(m_wal_offset + m_written == writer.cursor());
    return m_written + m_wal_offset;
};

void segment_file::append_footer(aligned_buf_writer &writer) {
    if (m_size - m_written < 8) {
        // 直接全填0
        // 直接略过了
        // writer->m_cursor += (m_size - m_written);
        // m_written = m_size;
        // assert(m_wal_offset + m_written == writer->cursor());
        uint64_t len = m_size - m_written;
        sslice s(std::string(len, '0'));
        writer.write(s);
        m_written += len;
        return;
    }

    uint32_t padding_length = m_size - m_written - 4 - 4;
    // 有 padding_length 个0
    sslice s(std::string(padding_length, '0'));
    // auto crc32 = cal_crc32(sslice(padding_length))
    auto length_type = cal_length_type(padding_length, 1);
    writer.write_u32(0);
    writer.write_u32(length_type);
    writer.write(s);
    m_written = m_size;
    assert(m_wal_offset + m_written == writer.cursor());
}

bool segment_file::open_and_create() {
    std::cout << "m_path = " << m_path << std::endl;
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

bool segment_file::can_hold(uint64_t size) {
    std::cout << "m_status = " << m_status << ", wal_offset = " << m_wal_offset
              << ",m_written = " << m_written << ",size = " << size << ", m_szie = " << m_size
              << std::endl;
    return (m_status == status::read_write) && (m_written + 8 + size <= m_size);
}
