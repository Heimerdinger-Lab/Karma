#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

#include "co_context/co/channel.hpp"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {
class echo_reply : public task {
   public:
    echo_reply(uint64_t from_id, uint64_t group_id, std::string msg)
        : m_from_id(from_id), m_group_id(group_id), m_msg(msg) {}
    ~echo_reply() = default;
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto msg = header_builder.CreateString(m_msg);
        auto header = karma_rpc::CreateEchoReply(header_builder, m_from_id, m_group_id, msg);
        header_builder.Finish(header);
        // set the frame
        uint8_t* buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_response();
        return ret_frame;
    };
    co_context::task<void> callback(transport::frame& reply_frame) override { co_return; };
    static std::unique_ptr<echo_reply> from_frame(transport::frame& frame) {
        std::string buffer_header = frame.m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::EchoReply>(buffer_header.data());
        std::cout << "from_frame, header: " << header->msg()->str() << std::endl;
        return std::make_unique<echo_reply>(header->from_id(), header->group_id(),
                                            header->msg()->str());
    };
    std::string msg() { return m_msg; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    std::string m_msg;
};

class echo_request : public task {
   public:
    echo_request(uint64_t from_id, uint64_t group_id, std::string msg)
        : m_from_id(from_id), m_group_id(group_id), m_msg(msg) {}
    ~echo_request() = default;
    void set_prom(co_context::channel<std::unique_ptr<echo_reply>>* prom) { m_prom = prom; }
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto msg = header_builder.CreateString(m_msg);
        auto header = karma_rpc::CreateEchoRequest(header_builder, m_from_id, m_group_id, msg);
        header_builder.Finish(header);
        // set the frame
        uint8_t* buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
    };
    static std::unique_ptr<echo_request> from_frame(transport::frame& frame) {
        std::string buffer_header = frame.m_header;
        std::cout << "buffer_header.size: " << buffer_header.size() << std::endl;
        auto header = flatbuffers::GetRoot<karma_rpc::EchoRequest>(buffer_header.data());
        std::cout << "from_frame, header: " << header->msg()->str() << std::endl;
        return std::make_unique<echo_request>(header->from_id(), header->group_id(),
                                              header->msg()->str());
        // return std::make_shared<echo_request>(0, 0, "123");
    };
    // 由frame生成request时，这时候往往在最上面，所以使用unique_ptr即可
    // 由frame生成reply时，也是unique_ptr，但是channel要变成unique_ptr
    co_context::task<void> callback(transport::frame& frame) override {
        co_await m_prom->release(echo_reply::from_frame(frame));
        co_return;
    };
    std::string msg() { return m_msg; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    std::string m_msg;
    co_context::channel<std::unique_ptr<echo_reply>>* m_prom;
};

}  // namespace client
