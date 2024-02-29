#pragma once
#include "co_context/co/channel.hpp"
#include "co_context/task.hpp"
#include "karma-client/error.h"
#include "karma-client/header.h"
// #include "karma-raft/raft.hh"
// #include "karma-raft/common.h"
// #include "karma-raft/rpc_message.h"
#include <flatbuffers/buffer.h>

#include <cstdint>
#include <memory>
#include <variant>

#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
namespace client {
class task {
   public:
    virtual std::shared_ptr<transport::frame> gen_frame() = 0;
    virtual co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) = 0;
};

// struct pong_pong_request {
//     std::string m_msg;
// };
// class ping_pong_task : public client_task {
// public:
//     ping_pong_task(raft::server_address start, raft::server_address target,
//     std::shared_ptr<pong_pong_request> req)
//         : client_task(start, target)
//         , m_req(req) {
//     }
//     ~ping_pong_task() = default;
//     std::shared_ptr<transport::frame> gen_frame() override {
//         auto ret =
//         std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_PING_PONG);
//         // header
//         flatbuffers::FlatBufferBuilder header_builder;
//         auto msg = header_builder.CreateString(m_req->m_msg);
//         auto sb = karma_rpc::CreatePingPongRequest(header_builder,
//         get_start_address().id, 0, msg); header_builder.Finish(sb); uint8_t*
//         buffer = header_builder.GetBufferPointer(); int size =
//         header_builder.GetSize(); ret->m_header.insert(ret->m_header.end(),
//         buffer, buffer + size); return ret;
//     };
//     co_context::task<void> callback(std::shared_ptr<transport::frame>
//     reply_frame) override {
//         co_return;
//     };
// private:
//     std::shared_ptr<pong_pong_request> m_req;
// };
};  // namespace client
