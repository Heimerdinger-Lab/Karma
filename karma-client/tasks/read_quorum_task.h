#include "protocol/rpc_generated.h"
#include "task.h"
#include <cstdint>
#include <variant>
#include <vector>

namespace client {
class read_quorum_reply : public task {
public:
  read_quorum_reply(uint64_t m_from_id, uint64_t m_group_id,
                    uint64_t m_current_term, uint64_t m_commit_idx,
                    uint64_t m_id)
      : m_from_id(m_from_id), m_group_id(m_group_id),
        m_current_term(m_current_term), m_commit_idx(m_commit_idx), m_id(m_id) {
  }
  std::shared_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_QUORUM);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header = karma_rpc::CreateReadQuorumReply(header_builder, m_from_id, m_group_id, m_current_term, m_commit_idx, m_id);
        header_builder.Finish(header);
        // set the frame
        uint8_t* buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_response();
        return ret_frame;
  }
  static std::shared_ptr<read_quorum_reply> from_frame(std::shared_ptr<transport::frame> frame) {
      std::string buffer_header = frame->m_header;
      auto header = flatbuffers::GetRoot<karma_rpc::ReadQuorumReply>(buffer_header.data());
      return std::make_shared<read_quorum_reply>(header->from_id(), header->group_id(), header->current_term(), header->commit_idx(), header->id());
  };
  co_context::task<void>
  callback(std::shared_ptr<transport::frame> reply_frame) override {}
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_current_term;
    uint64_t m_commit_idx;
    uint64_t m_id;
};

class read_quorum_request : public task {
public:
  read_quorum_request(uint64_t m_from_id, uint64_t m_group_id,
                      uint64_t m_current_term, uint64_t m_leader_commit_idx,
                      uint64_t m_id)
      : m_from_id(m_from_id), m_group_id(m_group_id),
        m_current_term(m_current_term),
        m_leader_commit_idx(m_leader_commit_idx), m_id(m_id) {}
  std::shared_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_QUORUM);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header = karma_rpc::CreateReadQuorum(header_builder, m_from_id, m_group_id, m_current_term, m_leader_commit_idx, m_id);
        header_builder.Finish(header);
        // set the frame
        uint8_t* buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_request();
        return ret_frame;
  }
  static std::shared_ptr<read_quorum_request> from_frame(std::shared_ptr<transport::frame> frame) {
      std::string buffer_header = frame->m_header;
      auto header = flatbuffers::GetRoot<karma_rpc::ReadQuorum>(buffer_header.data());
      return std::make_shared<read_quorum_request>(header->from_id(), header->group_id(), header->current_term(), header->leader_commit_idx(), header->id());
  };
  co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {

  }
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_current_term;
    uint64_t m_leader_commit_idx;
    uint64_t m_id;
};

}