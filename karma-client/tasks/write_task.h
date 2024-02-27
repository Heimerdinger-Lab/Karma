#include "protocol/rpc_generated.h"
#include "task.h"
#include <cstdint>
#include <sys/types.h>


namespace client {
class write_reply : public task {
public:
  explicit write_reply(bool m_success) : m_success(m_success) {}
  std::shared_ptr<transport::frame> gen_frame() override {
    auto ret_frame = std::make_shared<transport::frame>(
        karma_rpc::OperationCode::OperationCode_WRITE_TASK);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header = karma_rpc::CreateWriteReply(header_builder, m_success);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer,
                               buffer + size);
    ret_frame->flag_response();
    return ret_frame;
  };
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {
        co_return;
    };
private:
    bool m_success;
};
class write_request : public task {
public:
  write_request(
      uint64_t m_group_id, std::string m_key, std::string m_value,
      std::shared_ptr<co_context::channel<std::shared_ptr<write_reply>>> m_prom)
      : m_group_id(m_group_id), m_key(std::move(m_key)),
        m_value(std::move(m_value)), m_prom(std::move(m_prom)) {}
  std::shared_ptr<transport::frame> gen_frame() override {
    auto ret_frame = std::make_shared<transport::frame>(
        karma_rpc::OperationCode::OperationCode_WRITE_TASK);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto key = header_builder.CreateString(m_key);
    auto value = header_builder.CreateString(m_value);
    auto header =
        karma_rpc::CreateWriteRequest(header_builder, m_group_id, key, value);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer,
                               buffer + size);
    ret_frame->flag_request();
    return ret_frame;
  }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {
        co_return;
    };

private:
    uint64_t m_group_id;
    std::string m_key;
    std::string m_value;
    std::shared_ptr<co_context::channel<std::shared_ptr<write_reply>>> m_prom;
};
}