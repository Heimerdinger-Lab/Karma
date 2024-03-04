#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <variant>

#include "co_context/co/channel.hpp"
#include "karma-store/buf/aligned_buf.h"
namespace store {
// crc32(4 Bytes) + length_type(4 Bytes)
const int RECORD_HEADER_LENGTH = 4 + 4;
enum uring_op { read, write };
struct record_pos {
    uint64_t start_wal_offset;
    uint64_t record_size;
};
struct read_io_result {
    bool success;
};
struct write_io_result {
    bool success;
    uint64_t start_wal_offset;
    uint32_t size;
};
struct read_io {
    uint64_t start_wal_offset;
    uint32_t size;
    std::string &data;
    //
    co_context::channel<read_io_result> &m_prom;
    co_context::io_context &m_ctx;
};
struct write_io {
    std::span<char> data;
    //
    co_context::channel<write_io_result> &m_prom;
    co_context::io_context &m_ctx;
};

struct write_uring_context {
    // 整个buffer都刷下去
    // commit [buffer.wal_offset, buffer.limit())
    std::shared_ptr<aligned_buf> buffer;
};
struct read_uring_context {
    std::shared_ptr<aligned_buf> buffer;
    uint32_t read_seq;
};
struct uring_context {
    uring_op opcode;
    std::variant<write_uring_context, read_uring_context> detail;
};
};  // namespace store