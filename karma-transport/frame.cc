#include "frame.h"

#include <boost/log/trivial.hpp>
#include <memory>
#include <optional>

#include "karma-util/crc32c.h"
uint32_t transport::g_frame_id = 0;
transport::frame::frame(karma_rpc::OperationCode code) : m_operation_code(code) {
    m_seq = g_frame_id++;
}

size_t transport::frame::size() {
    return FIXED_HEADER_LENGTH + m_header.size() + m_data.size() + CRC32_LENGTH;
}

void transport::frame::flag_response() { m_flag = 1; }

void transport::frame::flag_request() { m_flag = 0; }

bool transport::frame::is_response() { return m_flag == 1; }

bool transport::frame::is_request() { return m_flag == 0; }

void transport::frame::set_header(std::string &header) { m_header = header; }

void transport::frame::set_payload(std::string &payload) { m_data = payload; }

/*
4 + 1 + 2 + 1 + 4 + 4 + xx + xx
- frame_length: u32
- magic_code: u8
- operation_code: i16
- flag: u8
- seq: u32
- Header_length: u32
- Header
- Payload
- crc32 of payload: u32
*/
std::string transport::frame::encode() {
    std::string encode_str;
    uint32_t header_length = m_header.length();
    uint32_t payload_length = m_data.length();
    uint32_t frame_length = FIXED_HEADER_LENGTH + header_length + payload_length + CRC32_LENGTH;
    encode_str.reserve(frame_length);
    encode_str.append((char *)&frame_length, (char *)&frame_length + 4);
    uint8_t code = MAGIC_CODE;
    encode_str.append((char *)&code, (char *)&code + 1);
    encode_str.append((char *)&m_operation_code, (char *)&m_operation_code + 2);
    encode_str.append((char *)&m_flag, (char *)&m_flag + 1);
    encode_str.append((char *)&m_seq, (char *)&m_seq + 4);
    encode_str.append((char *)&header_length, (char *)&header_length + 4);
    encode_str.append(m_header.begin(), m_header.end());
    encode_str.append(m_data.begin(), m_data.end());
    auto header_crc32 = crc32c::Value(m_header.data(), m_header.size());
    auto total_crc32 = crc32c::Extend(header_crc32, m_data.data(), m_data.size());
    encode_str.append((char *)&total_crc32, (char *)&total_crc32 + 4);
    // BOOST_LOG_TRIVIAL(debug) << "crc32: " << total_crc32;
    return encode_str;
};

std::optional<std::unique_ptr<transport::frame>> transport::frame::parse(std::span<char> src) {
    // assert(check(src));
    if (src.size() < FIXED_HEADER_LENGTH + CRC32_LENGTH) {
        return std::nullopt;
    }
    uint32_t check_offset = 0;
    uint32_t check_frame_size = *((uint32_t *)(src.data() + check_offset));
    check_offset += 4;
    if (check_frame_size > MAX_FRAME_SIZE) {
        throw std::runtime_error("Bad Frame: The decoded frame size is larger than the limit");
        return std::nullopt;
    }
    if (src.size() < check_frame_size) {
        return std::nullopt;
    }
    // Get an frame, start to decode it!
    auto ret = std::make_unique<transport::frame>();
    uint32_t offset = 0;
    uint32_t frame_size = *((uint32_t *)(src.data() + offset));
    offset += 4;
    uint8_t code = *((uint8_t *)(src.data() + offset));
    // assert(code == MAGIC_CODE);
    if (code != MAGIC_CODE) {
        throw std::runtime_error("Bad Frame: Wrong magic code");
        return std::nullopt;
    }
    offset += 1;
    uint16_t op_code = *((uint16_t *)(src.data() + offset));
    ret->m_operation_code = karma_rpc::OperationCode(op_code);
    offset += 2;
    uint8_t flag = *((uint8_t *)(src.data() + offset));
    ret->m_flag = flag;
    offset += 1;
    uint32_t seq = *((uint32_t *)(src.data() + offset));
    ret->m_seq = seq;
    offset += 4;
    uint32_t header_length = *((uint32_t *)(src.data() + offset));

    if (header_length > (frame_size - FIXED_HEADER_LENGTH - 4)) {
        throw std::runtime_error("Bad Frame: Wrong header length");
        return std::nullopt;
    }
    offset += 4;
    ret->m_header.reserve(header_length);
    ret->m_header.append(src.data() + offset, src.data() + offset + header_length);
    offset += header_length;
    if (frame_size - offset - 4 < 0) {
        throw std::runtime_error("Bad Frame: Wrong data length");
        return std::nullopt;
    }
    uint32_t body_length = frame_size - offset - 4;
    ret->m_data.reserve(body_length);
    ret->m_data.append(src.data() + offset, src.data() + offset + body_length);
    offset += body_length;
    uint32_t crc32 = *((uint32_t *)(src.data() + offset));
    auto header_crc32 = crc32c::Value(ret->m_header.data(), ret->m_header.size());
    auto total_crc32 = crc32c::Extend(header_crc32, ret->m_data.data(), ret->m_data.size());
    if (total_crc32 != crc32) {
        throw std::runtime_error("Bad Frame: Wrong crc32");
        return std::nullopt;
    }
    return ret;
};
