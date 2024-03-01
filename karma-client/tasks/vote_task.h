#pragma once
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {

class vote_request : public task {
   public:
    vote_request(uint64_t m_from_id, uint64_t m_group_id, raft::vote_request m_request)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_request(m_request) {}
    std::unique_ptr<transport::frame> gen_frame() override;
    static std::unique_ptr<vote_request> from_frame(transport::frame &frame);
    co_context::task<void> callback(transport::frame &reply_frame) override {}
    uint64_t from_id() { return m_from_id; }
    raft::vote_request request() { return m_request; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::vote_request m_request;
};
class vote_reply : public task {
   public:
    vote_reply(uint64_t m_from_id, uint64_t m_group_id, raft::vote_reply m_reply)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_reply(m_reply) {}
    std::unique_ptr<transport::frame> gen_frame() override;
    static std::unique_ptr<vote_reply> from_frame(transport::frame &frame);
    co_context::task<void> callback(transport::frame &reply_frame) override {}
    uint64_t from_id() { return m_from_id; }
    raft::vote_reply reply() { return m_reply; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::vote_reply m_reply;
};
}  // namespace client
