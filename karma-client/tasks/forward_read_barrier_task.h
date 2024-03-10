#pragma once
#include "karma-client/tasks/task.h"
#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
namespace client {
class forward_read_barrier_task_reply : public task {
   public:
    forward_read_barrier_task_reply(uint64_t m_from_id, uint64_t m_group_id,
                                    raft::read_barrier_reply m_reply)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_reply(std::move(m_reply)) {}
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_unique<transport::frame>(
            karma_rpc::OperationCode::OperationCode_FORWARD_APPEND_ENTRY);
        // header
        flatbuffers::FlatBufferBuilder header_builder;

        bool success = m_reply.index() == 1;
        uint64_t index = success ? std::get<raft::index_t>(m_reply) : 0;
        auto header = karma_rpc::CreateForwardReadBarrierReply(header_builder, m_from_id,
                                                               m_group_id, success, index);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
    }
    co_context::task<void> callback(transport::frame &reply_frame) override { co_return; };

    static std::unique_ptr<forward_read_barrier_task_reply> from_frame(transport::frame &frame) {
        std::string &buffer_header = frame.m_header;
        auto header =
            flatbuffers::GetRoot<karma_rpc::ForwardReadBarrierReply>(buffer_header.data());
        // BOOST_LOG_TRIVIAL(trace) << "Generate an echo reply from frame";

        if (header->success()) {
            raft::index_t index = header->index();
            return std::make_unique<forward_read_barrier_task_reply>(header->from_id(),
                                                                     header->group_id(), index);
        } else {
            return std::make_unique<forward_read_barrier_task_reply>(
                header->from_id(), header->group_id(), std::monostate{});
        }
    }
    bool success() { return m_reply.index() == 1; }
    raft::index_t reply() { return std::get<raft::index_t>(m_reply); }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::read_barrier_reply m_reply;
};

class forward_read_barrier_task : public task {
   public:
    forward_read_barrier_task(uint64_t m_from_id, uint64_t m_group_id)
        : m_from_id(m_from_id), m_group_id(m_group_id) {}
    void set_prom(co_context::channel<std::unique_ptr<forward_read_barrier_task_reply>> *prom) {
        m_prom = prom;
    }
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header = karma_rpc::CreateForwardReadBarrier(header_builder, m_from_id, m_group_id);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
    }
    co_context::task<void> callback(transport::frame &reply_frame) override {
        // one way rpc do not need to implement it.
        co_await m_prom->release(
            std::move(forward_read_barrier_task_reply::from_frame(reply_frame)));
        co_return;
    }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    co_context::channel<std::unique_ptr<forward_read_barrier_task_reply>> *m_prom;
};
}  // namespace client