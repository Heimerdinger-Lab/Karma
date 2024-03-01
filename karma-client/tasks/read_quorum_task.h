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
    std::unique_ptr<transport::frame> gen_frame() override;
    static std::unique_ptr<read_quorum_reply> from_frame(transport::frame &frame);
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
    std::unique_ptr<transport::frame> gen_frame() override;
    static std::unique_ptr<read_quorum_request> from_frame(transport::frame &frame);
    co_context::task<void> callback(transport::frame &reply_frame) override {}
    uint64_t from_id() { return m_from_id; }
    raft::read_quorum request() { return m_request; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::read_quorum m_request;
};

}  // namespace client