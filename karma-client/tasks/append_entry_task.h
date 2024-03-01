#pragma once
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {
class append_entry_reply : public task {
   public:
    append_entry_reply(uint64_t m_from_id, uint64_t m_group_id, raft::append_reply m_reply)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_reply(m_reply) {}
    std::unique_ptr<transport::frame> gen_frame() override;
    co_context::task<void> callback(transport::frame &reply_frame) override {}
    static std::unique_ptr<append_entry_reply> from_frame(transport::frame &frame);
    uint64_t from_id() { return m_from_id; }
    raft::append_reply reply() { return m_reply; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::append_reply m_reply;
};
class append_entry_request : public task {
   public:
    append_entry_request(uint64_t m_from_id, uint64_t m_group_id, raft::append_request m_request)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_request(std::move(m_request)) {}
    std::unique_ptr<transport::frame> gen_frame() override;
    static std::unique_ptr<append_entry_request> from_frame(transport::frame &frame);
    co_context::task<void> callback(transport::frame &reply_frame) override {}
    uint64_t from_id() { return m_from_id; }
    raft::append_request request() { return m_request; }

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    raft::append_request m_request;
};
}  // namespace client