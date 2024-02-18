#pragma once
#include "co_context/co/channel.hpp"
#include "co_context/task.hpp"
#include "karma-client/error.h"
#include "karma-client/header.h"
#include "scylladb-raft/raft.hh"
// #include "karma-raft/common.h"
// #include "karma-raft/rpc_message.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#include <flatbuffers/buffer.h>
#include <memory>
#include <variant>
namespace client {
class client_task {
public:
    client_task(raft::server_address start, raft::server_address target)
        : m_start(start)
        , m_target(target) {
    }
    raft::server_address get_start_address() {
        return m_start;
    };
    raft::server_address get_target_address() {
        return m_target;
    };
    virtual std::shared_ptr<transport::frame> gen_frame() = 0;
    virtual co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) = 0;
private:
    raft::server_address m_start;
    raft::server_address m_target;
};
// class ping_pong_task : public client_task {
// public:
//     ping_pong_task(raft::server_address start, raft::server_address target, std::shared_ptr<raft::ping_pong_request> req, std::shared_ptr<co_context::channel<std::variant<raft::ping_pong_reply, client_error>, 0>> channel) 
//         : client_task(start, target)
//         , m_req(req) 
//         , m_channel(channel){
//     }
//     ~ping_pong_task() = default;
//     std::shared_ptr<transport::frame> gen_frame() override {
//         auto ret = std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_PING_PONG);
//         // header
//         flatbuffers::FlatBufferBuilder header_builder;
//         auto msg = header_builder.CreateString(m_req->m_msg);
//         auto sb = karma_rpc::CreatePingPongRequest(header_builder, get_start_address().id, 0, msg);
//         header_builder.Finish(sb);
//         uint8_t* buffer = header_builder.GetBufferPointer();
//         int size = header_builder.GetSize();
//         ret->m_header.insert(ret->m_header.end(), buffer, buffer + size);
//         return ret;
//     };
//     co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {
        
//         auto header = flatbuffers::GetRoot<karma_rpc::PingPongReply>(reply_frame->m_header.data());
//         std::cout << "callback!!!" << header->msg()->str() << ", size " << header->msg()->str().size() << std::endl;
        
//         raft::ping_pong_reply r{.m_msg = std::string(header->msg()->str())};
//         std::cout << "r.msg: " << r.m_msg << std::endl;
//         co_await m_channel->release(r);
//     };
// private:
//     std::shared_ptr<raft::ping_pong_request> m_req;
//     std::shared_ptr<co_context::channel<std::variant<raft::ping_pong_reply, client_error>, 0>> m_channel;
// };
};
