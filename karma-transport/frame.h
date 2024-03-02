#pragma once
#include <sys/types.h>

#include <cstdint>
#include <string>

#include "protocol/rpc_generated.h"
// 它负责数据的释放
namespace transport {
extern uint32_t g_frame_id;
class frame {
   public:
    karma_rpc::OperationCode m_operation_code = karma_rpc::OperationCode::OperationCode_UNKNOW;
    uint8_t m_flag = 0;
    uint32_t m_seq = 0;
    std::string m_header;
    std::string m_data;

   public:
    static const uint8_t MAGIC_CODE = 123;
    static const uint32_t FIXED_HEADER_LENGTH = (4 + 1 + 2 + 1 + 4 + 4);
    static const uint32_t CRC32_LENGTH = 4;
    static const uint32_t MAX_FRAME_SIZE = 4096 * 128;

   public:
    frame() = default;
    frame(karma_rpc::OperationCode code);
    size_t size();
    void flag_response();
    void flag_request();
    bool is_response();
    bool is_request();
    void set_header(std::string& header);
    void set_payload(std::string& payload);
    std::string encode();
    static std::optional<std::unique_ptr<transport::frame>> parse(std::span<char> src);
};
}  // namespace transport
