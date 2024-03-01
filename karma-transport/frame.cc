#include "frame.h"

#include <memory>
std::atomic_int64_t transport::g_frame_id = 0;
transport::frame::frame(karma_rpc::OperationCode code) : m_operation_code(code) {
    m_seq = g_frame_id++;
}

size_t transport::frame::size() {
    return FIXED_HEADER_LENGTH + m_header.size() + m_data.size() + 4;
}

void transport::frame::flag_response() { m_flag = 1; }

void transport::frame::flag_request() { m_flag = 0; }

bool transport::frame::is_response() { return m_flag == 1; }

bool transport::frame::is_request() { return m_flag == 0; }

void transport::frame::set_header(std::string header) { m_header = header; }

void transport::frame::set_payload(std::string payload) { m_data = payload; }

std::string transport::frame::encode() {
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
    std::string encode_str;
    uint32_t extend_header_length = m_header.length();
    uint32_t payload_length = m_data.length();
    // FIXED HEADER + EXTEND HEADER + PAYLOAD + PAYLOAD CRC32
    uint32_t frame_length = FIXED_HEADER_LENGTH + extend_header_length + payload_length + 4;
    std::cout << "encode frame length = " << frame_length
              << ", header_length: " << extend_header_length << ", payload: " << payload_length
              << std::endl;
    encode_str.reserve(frame_length);
    encode_str.insert(encode_str.end(), (char *)&frame_length, (char *)&frame_length + 4);
    uint8_t code = MAGIC_CODE;
    encode_str.insert(encode_str.end(), (char *)&code, (char *)&code + 1);
    encode_str.insert(encode_str.end(), (char *)&m_operation_code, (char *)&m_operation_code + 2);
    encode_str.insert(encode_str.end(), (char *)&m_flag, (char *)&m_flag + 1);
    std::cout << "seq = " << m_seq << std::endl;
    encode_str.insert(encode_str.end(), (char *)&m_seq, (char *)&m_seq + 4);
    encode_str.insert(encode_str.end(), (char *)&extend_header_length,
                      (char *)&extend_header_length + 4);
    encode_str.insert(encode_str.end(), m_header.begin(), m_header.end());
    encode_str.insert(encode_str.end(), m_data.begin(), m_data.end());
    uint32_t crc32 = TEMP_CRC32;
    encode_str.insert(encode_str.end(), (char *)&crc32, (char *)&crc32 + 4);
    return encode_str;
};

std::unique_ptr<transport::frame> transport::frame::parse(std::span<char> src) {
    // frame ret{};
    auto ret = std::make_unique<transport::frame>();
    uint32_t offset = 0;
    uint32_t frame_size = *((uint32_t *)(src.data() + offset));
    std::cout << "frame_size: " << frame_size << std::endl;
    offset += 4;
    // skip the magic code
    offset += 1;
    uint16_t op_code = *((uint16_t *)(src.data() + offset));
    std::cout << "op = " << op_code << std::endl;
    ret->m_operation_code = karma_rpc::OperationCode(op_code);
    offset += 2;
    std::cout << "offset = " << offset << std::endl;
    uint8_t xxx = *((uint8_t *)(src.data() + offset));
    std::cout << "flag = " << ((int)xxx + 0) << std::endl;
    ret->m_flag = xxx;
    offset += 1;
    uint32_t seq = *((uint32_t *)(src.data() + offset));
    std::cout << "seq = " << seq << std::endl;
    ret->m_seq = seq;
    offset += 4;
    uint32_t header_length = *((uint32_t *)(src.data() + offset));
    offset += 4;
    // ret->m_header.reserve(header_length);
    ret->m_header.insert(ret->m_header.end(), src.data() + offset,
                         src.data() + offset + header_length);
    // memcpy(ret.m_header.data(), src.data() + offset, header_length);
    offset += header_length;
    std::cout << "offset = " << offset << ", header_length = " << header_length << std::endl;
    uint32_t body_length = frame_size - offset - 4;
    std::cout << "body_length = " << body_length << std::endl;
    // ret->m_data.reserve(body_length);
    // memcpy(ret.m_data.data(), src.data() + offset, body_length);
    ret->m_data.insert(ret->m_data.end(), src.data() + offset, src.data() + offset + body_length);
    // skip the crc32
    return ret;
};
void transport::frame::check(std::span<char> src) {
    std::cout << "span size = " << src.size() << std::endl;
    if (src.size() < 4) {
        throw frame_error::incomplete();
    }
    uint32_t offset = 0;
    uint32_t frame_size = *((uint32_t *)(src.data() + offset));
    offset += 4;
    if (frame_size > src.size()) {
        throw frame_error::incomplete();
    }
    uint8_t magic = *((uint8_t *)(src.data() + offset));
    offset += 1;
    if (magic != MAGIC_CODE) {
        throw frame_error::bad_frame();
    }
    // pass the op_code
    offset += 2;
    // pass the flag
    offset += 1;
    // pass the seq
    offset += 4;
    // header_length
    uint32_t header_length = *((uint32_t *)(src.data() + offset));
    offset += 4 + header_length;
    uint32_t body_length = frame_size - offset - 4;
    if (body_length < 0) {
        throw frame_error::bad_frame();
    }
    // check crc32
    offset += body_length;
    uint32_t crc32 = *((uint32_t *)(src.data() + offset));
    if (crc32 != TEMP_CRC32) {
        throw frame_error::bad_frame();
    }
};