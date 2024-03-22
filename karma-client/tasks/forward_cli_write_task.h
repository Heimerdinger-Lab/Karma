#pragma once
#include <cmath>

#include "karma-client/tasks/cli_write_task.h"
#include "karma-client/tasks/task.h"
#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
namespace client {

class forward_cli_write_task_reply : public task {
   public:
    forward_cli_write_task_reply(uint64_t m_from_id, uint64_t m_group_id, cli_write_reply m_reply)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_reply(std::move(m_reply)) {}
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_unique<transport::frame>(
            karma_rpc::OperationCode::OperationCode_FORWARD_CLI_WRITE);
        // header
        flatbuffers::FlatBufferBuilder header_builder;

        bool success = m_reply.success();
        auto header =
            karma_rpc::CreateForwardCliWriteReply(header_builder, m_from_id, m_group_id, success);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_response();
        return ret_frame;
    }
    co_context::task<void> callback(transport::frame &reply_frame) override { co_return; };
    static std::unique_ptr<forward_cli_write_task_reply> from_frame(transport::frame &frame) {
        std::string &buffer_header = frame.m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::ForwardCliWriteReply>(buffer_header.data());
        // BOOST_LOG_TRIVIAL(trace) << "Generate an echo reply from frame";
        return std::make_unique<forward_cli_write_task_reply>(header->from_id(), header->group_id(),
                                                              cli_write_reply(header->success()));
    }
    bool success() { return m_reply.success(); }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    cli_write_reply m_reply;
};

class forward_cli_write_task : public task {
   public:
    forward_cli_write_task(uint64_t m_from_id, uint64_t m_group_id, std::string m_key,
                           std::string m_value)
        : m_from_id(m_from_id),
          m_group_id(m_group_id),
          m_key(std::move(m_key)),
          m_value(std::move(m_value)) {}
    void set_prom(co_context::channel<std::unique_ptr<forward_cli_write_task_reply>> *prom) {
        m_prom = prom;
    }
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_unique<transport::frame>(
            karma_rpc::OperationCode::OperationCode_FORWARD_CLI_WRITE);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto key = header_builder.CreateString(m_key);
        auto value = header_builder.CreateString(m_value);
        auto command =
            karma_rpc::CreateCommand(header_builder, karma_rpc::CommandType_VALUE, key, value);
        auto header =
            karma_rpc::CreateForwardCliWrite(header_builder, m_from_id, m_group_id, command);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
    }
    static std::unique_ptr<forward_cli_write_task> from_frame(transport::frame &frame) {
        std::string &buffer_header = frame.m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::ForwardCliWrite>(buffer_header.data());
        // BOOST_LOG_TRIVIAL(trace) << "Generate an echo reply from frame";
        return std::make_unique<forward_cli_write_task>(header->from_id(), header->group_id(),
                                                        header->command()->key()->str(),
                                                        header->command()->value()->str());
    }
    co_context::task<void> callback(transport::frame &reply_frame) override {
        // one way rpc do not need to implement it.
        co_await m_prom->release(std::move(forward_cli_write_task_reply::from_frame(reply_frame)));
        co_return;
    }
    std::string key() { return m_key; }
    std::string value() { return m_value; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    std::string m_key;
    std::string m_value;
    co_context::channel<std::unique_ptr<forward_cli_write_task_reply>> *m_prom;
};

};  // namespace client