#include "protocol/rpc_generated.h"
#include "task.h"
#include <cstdint>
#include <variant>
#include <vector>

namespace client {


struct append_entry_accepted {
    uint64_t last_new_idx;
};
struct append_entry_rejected {
    uint64_t non_matching_idx;
    uint64_t last_idx;
};

class append_entry_reply : public task {
public:
  append_entry_reply(
      uint64_t m_from_id, uint64_t m_group_id, uint64_t m_term,
      uint64_t m_index,
      std::variant<append_entry_accepted, append_entry_rejected> m_result)
      : m_from_id(m_from_id), m_group_id(m_group_id), m_term(m_term),
        m_index(m_index), m_result(m_result) {}
  std::shared_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_APPEND_ENTRY);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        if (m_result.index() == 0) {
            auto result = std::get<append_entry_accepted>(m_result);
            auto accepted = karma_rpc::CreateAppendEntryAccepted(header_builder, result.last_new_idx);
            auto header = karma_rpc::CreateAppendEntryReply(header_builder, m_from_id, m_group_id, m_term, m_index, karma_rpc::AppendEntryResult_AppendEntryAccepted, accepted.Union());
            header_builder.Finish(header);
        } else {
            auto result = std::get<append_entry_rejected>(m_result);
            auto rejected = karma_rpc::CreateAppendEntryRejected(header_builder, result.non_matching_idx, result.last_idx);
            auto header = karma_rpc::CreateAppendEntryReply(header_builder, m_from_id, m_group_id, m_term, m_index, karma_rpc::AppendEntryResult_AppendEntryRejected, rejected.Union());
            header_builder.Finish(header);
        }
        uint8_t* buffer = header_builder.GetBufferPointer();
        int size = header_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
        ret_frame->flag_response();
        return ret_frame;
  }
  co_context::task<void>
  callback(std::shared_ptr<transport::frame> reply_frame) override {}
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_term;
    uint64_t m_index;
    std::variant<append_entry_accepted, append_entry_rejected> m_result;

};

struct log_entry {
    uint64_t term;
    uint64_t index;
    std::string command;
};
class append_entry_request : public task {
public:
  append_entry_request(uint64_t m_from_id, uint64_t m_group_id,
                       uint64_t m_current_term, uint64_t m_prev_log_idx,
                       uint64_t m_prev_log_term, uint64_t m_leader_commit_idx,
                       std::vector<log_entry> m_entries)
      : m_from_id(m_from_id), m_group_id(m_group_id),
        m_current_term(m_current_term), m_prev_log_idx(m_prev_log_idx),
        m_prev_log_term(m_prev_log_term),
        m_leader_commit_idx(m_leader_commit_idx),
        m_entries(std::move(m_entries)) {}
  std::shared_ptr<transport::frame> gen_frame() override {
        auto ret_frame = std::make_shared<transport::frame>(karma_rpc::OperationCode::OperationCode_APPEND_ENTRY);
        // header
        flatbuffers::FlatBufferBuilder header_builder;
        auto header = karma_rpc::CreateAppendEntryRequest(header_builder, m_from_id, m_group_id, m_current_term, m_prev_log_idx, m_prev_log_term, m_leader_commit_idx);
        header_builder.Finish(header);

        // body 
        flatbuffers::FlatBufferBuilder body_builder;
        std::vector<::flatbuffers::Offset<karma_rpc::LogEntry>> log_entries;
        for (auto& item : m_entries) {
            auto command = body_builder.CreateString(item.command);
            auto log_entry = karma_rpc::CreateLogEntry(body_builder, item.term, item.index, command);
            log_entries.push_back(log_entry);
        }
        auto payload = karma_rpc::CreateAppendEntryPayload(body_builder, body_builder.CreateVector(log_entries));
        body_builder.Finish(payload);
        // set the frame
        uint8_t* header_buffer = header_builder.GetBufferPointer();
        uint8_t* body_buffer = body_builder.GetBufferPointer();
        int header_size = header_builder.GetSize();
        int body_size = body_builder.GetSize();
        ret_frame->m_header.insert(ret_frame->m_header.end(), header_buffer, header_buffer + header_size);
        ret_frame->m_data.insert(ret_frame->m_data.end(), body_buffer, body_buffer + body_size);
        ret_frame->flag_request();
        return ret_frame;
  }
    static std::shared_ptr<append_entry_request> from_frame(std::shared_ptr<transport::frame> frame) {
        auto buffer_header = frame->m_header;
        auto body = frame->m_data;
        auto header = flatbuffers::GetRoot<karma_rpc::AppendEntryRequest>(buffer_header.data());
        // std::cout << "from_frame, header: " << header->msg()->str() << std::endl;
        // payload
        auto payload = flatbuffers::GetRoot<karma_rpc::AppendEntryPayload>(body.data());
        std::vector<log_entry> entries;
        auto log_entries = payload->entries();
        for (flatbuffers::uoffset_t i = 0; i < log_entries->size(); ++i) {
            auto log_entry = log_entries->Get(i);

            // 读取LogEntry字段的值
            int64_t term = log_entry->term();
            int64_t index = log_entry->index();
            std::string command = log_entry->command()->str();

            // 打印LogEntry的字段值
            std::cout << "LogEntry " << i << ": " << term << ", " << index << ", " << command << std::endl;
            struct log_entry entry;
            entry.index = index;
            entry.term = term;
            entry.command = command;
            entries.push_back(entry);
        }
        //
        return std::make_shared<append_entry_request>(header->from_id(), header->group_id(), header->current_term(), header->prev_log_idx(), header->prev_log_term(), header->leader_commit_idx(), entries);
    };
  co_context::task<void>callback(std::shared_ptr<transport::frame> reply_frame) override {

  }
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_current_term;
    uint64_t m_prev_log_idx;
    uint64_t m_prev_log_term;
    uint64_t m_leader_commit_idx;
    std::vector<log_entry> m_entries;
};
}