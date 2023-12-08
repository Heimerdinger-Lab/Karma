#pragma once
#include <iostream>
#include "protocol/rpc_generated.h"
#include "error.h"
// 它负责数据的释放
namespace transport {
class frame {
public:
    frame() {

    };
    std::string encode() {
        /*
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
    };
    static frame parse(std::span<char> src) {
        frame ret;
        
        uint32_t offset = 0;
        uint32_t frame_size = *((uint32_t *)(src.data() + offset));
        offset += 4;
        // skip the magic code
        offset += 1;
        uint16_t op_code = *((uint16_t *)(src.data() + offset));
        ret.m_operation_code = karma_rpc::OperationCode(op_code);
        offset += 2;
        uint8_t flag = *((uint8_t *)(src.data() + offset));
        ret.m_flag = flag;
        offset += 1;
        uint32_t seq = *((uint32_t *)(src.data() + offset));
        ret.m_seq = seq;
        offset += 4;
        uint32_t header_length = *((uint32_t *)(src.data() + offset));
        offset += 4;
        ret.m_header.reserve(header_length);
        memcpy(ret.m_header.data(), src.data() + offset, header_length);
        offset += header_length;
        uint32_t body_length = frame_size - offset - 4;
        ret.m_data.reserve(body_length);
        memcpy(ret.m_data.data(), src.data() + offset, body_length);

        return ret;
    };
    static void check(std::span<char> src) {
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
        if (crc32 != 6666) {
            throw frame_error::bad_frame();
        }
    };
private:
    static const uint8_t MAGIC_CODE = 123;
    karma_rpc::OperationCode m_operation_code; 
    uint8_t m_flag;
    uint8_t m_seq;
    std::string m_header;
    std::string m_data;
};
}
