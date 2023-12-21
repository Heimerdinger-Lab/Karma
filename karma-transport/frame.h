#pragma once
#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <sys/types.h>
#include "protocol/rpc_generated.h"
#include "error.h"
// 它负责数据的释放
namespace transport {
extern std::atomic_int64_t g_frame_id;
class frame {
public:
    karma_rpc::OperationCode m_operation_code = karma_rpc::OperationCode::OperationCode_PING_PONG; 
    uint8_t m_flag = 0;
    uint32_t m_seq = 0;
    std::string m_header = "";
    std::string m_data = "";
public:
     static const uint8_t MAGIC_CODE = 123;
     static const uint32_t TEMP_CRC32 = 666;
     static const uint32_t FIXED_HEADER_LENGTH = (4 + 1 + 2 + 1 + 4 + 4);
public:
    frame() = default;
    frame(karma_rpc::OperationCode code);
    size_t size();
    void flag_response();
    void flag_request();
    bool is_response();
    bool is_request();
    void set_header(std::string header);
    void set_payload(std::string payload);
    std::string encode();
    static std::shared_ptr<frame> parse(std::span<char> src);
    static void check(std::span<char> src);
};
}
