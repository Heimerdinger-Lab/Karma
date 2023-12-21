#pragma once
#include "co_context/co/channel.hpp"
#include "co_context/task.hpp"
#include "karma-raft/rpc_message.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#include <memory>
namespace service {
class handler {
public:
    virtual co_context::task<void> call() = 0;
};
class ping_pong_handler : public handler {
public:
    ping_pong_handler(std::shared_ptr<transport::frame> f, std::shared_ptr<co_context::channel<std::shared_ptr<transport::frame>>> channel)
        : m_frame(f)
        , m_channel(channel) {
            printf("%p in handler, count: %ld\n", m_frame->m_header.data(), m_frame.use_count());

        }

    co_context::task<void> call() override {
        printf("%p in call, count: %ld\n", m_frame->m_header.data(), m_frame.use_count());
        std::string buffer_header = m_frame->m_header;
        printf("%p | %p\n", m_frame->m_header.data(), buffer_header.data());
        
        // std::cout << "buffer_header: " << buffer_header << std::endl;
        auto header = flatbuffers::GetRoot<karma_rpc::PingPongRequest>(buffer_header.data());
        // bool isValid = (flatbuffers::Verifier(buffer_header.data(), buffer_header.size()));
        flatbuffers::Verifier verifier((const uint8_t *const)buffer_header.data(), buffer_header.size());
        std::cout << "verify: " << header->Verify(verifier) << std::endl;
        std::cout << "handle.from_id: " << header->from_id() << std::endl;
        std::cout << "Handler: I have receive msg: " << header->msg()->str() << std::endl;
        // std::cout << "Handler: I have receive msg: " << m_frame->m_header << std::endl;
        raft::ping_pong_request req {
            .m_msg = header->msg()->c_str()
        };

        auto reply = work(req);
        
        flatbuffers::FlatBufferBuilder header_builder;
        auto msg = header_builder.CreateString(reply.m_msg);
        auto sb = karma_rpc::CreatePingPongReply(header_builder, 23, 0, msg);
        header_builder.Finish(sb);

        // create response frame
        auto f = std::make_shared<transport::frame>();
        f->m_operation_code = karma_rpc::OperationCode_PING_PONG;
        f->flag_response();
        uint8_t* buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        f->m_header.insert(f->m_header.end(), buffer, buffer + size);
        co_await m_channel->release(f);
        
    }
    raft::ping_pong_reply work(raft::ping_pong_request req) {
        return raft::ping_pong_reply {
            .m_msg = "ok, i have receive your msg that is " + req.m_msg
        };
    }
private:
    std::shared_ptr<transport::frame> m_frame;
    std::shared_ptr<co_context::channel<std::shared_ptr<transport::frame>>> m_channel;
};
};