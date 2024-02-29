#pragma once
#include <sys/types.h>

#include <cstdint>

#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {
class write_reply : public task {
   public:
    explicit write_reply(bool m_success) : m_success(m_success) {}
    std::shared_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_WRITE_TASK);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header = karma_rpc::CreateWriteReply(header_builder, m_success);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_response();
        return ret_frame;
    };
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {
        co_return;
    };
    static std::shared_ptr<write_reply> from_frame(std::shared_ptr<transport::frame> frame) {
        std::string buffer_header = frame->m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::WriteReply>(buffer_header.data());
        return std::make_shared<write_reply>(header->success());
    };
    bool success() { return m_success; }

   private:
    bool m_success;
};
class write_request : public task {
   public:
    write_request(uint64_t m_group_id, std::string m_key, std::string m_value)
        : m_group_id(m_group_id), m_key(std::move(m_key)), m_value(std::move(m_value)) {}
    void set_prom(std::shared_ptr<co_context::channel<std::shared_ptr<write_reply>>> prom) {
        m_prom = prom;
    }
    std::shared_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_WRITE_TASK);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto key = header_builder.CreateString(m_key);
        auto value = header_builder.CreateString(m_value);
        auto header = karma_rpc::CreateWriteRequest(header_builder, m_group_id, key, value);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
    }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {
        std::cout << "callback1" << std::endl;
        auto reply = write_reply::from_frame(reply_frame);
        std::cout << "callback2" << std::endl;
        co_await m_prom->release(reply);
        std::cout << "callback3" << std::endl;
        co_return;
    };
    static std::shared_ptr<write_request> from_frame(std::shared_ptr<transport::frame> frame) {
        std::string buffer_header = frame->m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::WriteRequest>(buffer_header.data());
        return std::make_shared<write_request>(header->group_id(), header->key()->str(),
                                               header->value()->str());
    };
    std::string key() { return m_key; }
    std::string value() { return m_value; }

   private:
    uint64_t m_group_id;
    std::string m_key;
    std::string m_value;
    std::shared_ptr<co_context::channel<std::shared_ptr<write_reply>>> m_prom;
};
}  // namespace client