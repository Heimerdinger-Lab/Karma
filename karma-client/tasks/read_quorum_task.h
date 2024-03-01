#pragma once
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {
class read_quorum_reply : public task {
   public:
    read_quorum_reply(uint64_t m_from_id, uint64_t m_group_id, raft::read_quorum_reply m_reply)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_reply(m_reply) {}
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_QUORUM);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header =
            karma_rpc::CreateReadQuorumReply(header_builder, m_from_id, m_group_id,
                                             m_reply.current_term, m_reply.commit_idx, m_reply.id);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_response();
        return ret_frame;
    }
    static std::unique_ptr<read_quorum_reply> from_frame(transport::frame &frame) {
        std::string buffer_header = frame.m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::ReadQuorumReply>(buffer_header.data());
        raft::read_quorum_reply reply{
            .current_term = static_cast<raft::term_t>(header->current_term()),
            .commit_idx = static_cast<raft::index_t>(header->commit_idx()),
            .id = static_cast<raft::read_id>(header->id())};
        return std::make_unique<read_quorum_reply>(header->from_id(), header->group_id(), reply);
    };
    co_context::task<void> callback(transport::frame &reply_frame) override {}

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    // uint64_t m_current_term;
    // uint64_t m_commit_idx;
    // uint64_t m_id;
    raft::read_quorum_reply m_reply;
};

class read_quorum_request : public task {
   public:
    read_quorum_request(uint64_t m_from_id, uint64_t m_group_id, raft::read_quorum m_request)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_request(m_request) {}
    std::unique_ptr<transport::frame> gen_frame() override {
        auto ret_frame =
            std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_QUORUM);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header = karma_rpc::CreateReadQuorum(header_builder, m_from_id, m_group_id,
                                                  m_request.current_term,
                                                  m_request.leader_commit_idx, m_request.id);
        header_builder.Finish(header);
        // set the frame
        uint8_t *buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
    }
    static std::unique_ptr<read_quorum_request> from_frame(transport::frame &frame) {
        std::string buffer_header = frame.m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::ReadQuorum>(buffer_header.data());
        raft::read_quorum request{
            .current_term = static_cast<raft::term_t>(header->current_term()),
            .leader_commit_idx = static_cast<raft::index_t>(header->leader_commit_idx()),
            .id = static_cast<raft::read_id>(header->id())};
        return std::make_unique<read_quorum_request>(header->from_id(), header->group_id(),
                                                     request);
    };
    co_context::task<void> callback(transport::frame &reply_frame) override {}
    uint64_t from_id() { return m_from_id; }
    raft::read_quorum request() { return m_request; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::read_quorum m_request;
};

}  // namespace client