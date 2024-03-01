#include "append_entry_task.h"
std::unique_ptr<client::append_entry_request> client::append_entry_request::from_frame(
    transport::frame &frame) {
    auto buffer_header = frame.m_header;
    auto body = frame.m_data;
    auto header = flatbuffers::GetRoot<karma_rpc::AppendEntryRequest>(buffer_header.data());
    // std::cout << "from_frame, header: " << header->msg()->str() << std::endl;
    // payload
    auto payload = flatbuffers::GetRoot<karma_rpc::AppendEntryPayload>(body.data());
    std::vector<raft::log_entry_ptr> entries;
    auto log_entries = payload->entries();
    for (flatbuffers::uoffset_t i = 0; i < log_entries->size(); ++i) {
        auto log_entry = log_entries->Get(i);

        // 读取LogEntry字段的值
        int64_t term = log_entry->term();
        int64_t index = log_entry->index();
        std::string command = log_entry->command()->str();

        // 打印LogEntry的字段值
        std::cout << "LogEntry " << i << ": " << term << ", " << index << ", " << command
                  << std::endl;
        //   struct log_entry entry;
        raft::log_entry_ptr entry = std::make_shared<raft::log_entry>();
        entry->idx = index;
        entry->term = term;
        if (command.size() == 0) {
            entry->data = raft::log_entry::dummy{};
        } else {
            entry->data = command;
        }
        entries.push_back(entry);
    }
    //
    raft::append_request request{
        .current_term = static_cast<raft::term_t>(header->current_term()),
        .prev_log_idx = static_cast<raft::index_t>(header->prev_log_idx()),
        .prev_log_term = static_cast<raft::term_t>(header->prev_log_term()),
        .leader_commit_idx = static_cast<raft::index_t>(header->leader_commit_idx()),
        .entries = entries};

    return std::make_unique<append_entry_request>(header->from_id(), header->group_id(), request);
};

std::unique_ptr<transport::frame> client::append_entry_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_APPEND_ENTRY);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header = karma_rpc::CreateAppendEntryRequest(
        header_builder, m_from_id, m_group_id, m_request.current_term, m_request.prev_log_idx,
        m_request.prev_log_term, m_request.leader_commit_idx);
    header_builder.Finish(header);

    // body
    flatbuffers::FlatBufferBuilder body_builder;
    std::vector<::flatbuffers::Offset<karma_rpc::LogEntry>> log_entries;
    for (auto &item : m_request.entries) {
        if (item->data.index() == 0) {
            auto command = body_builder.CreateString(std::get<raft::command>(item->data));
            auto log_entry =
                karma_rpc::CreateLogEntry(body_builder, item->term, item->idx, command);
            log_entries.push_back(log_entry);
        } else {
            auto command = body_builder.CreateString("");
            auto log_entry =
                karma_rpc::CreateLogEntry(body_builder, item->term, item->idx, command);
            log_entries.push_back(log_entry);
        }
    }
    auto payload =
        karma_rpc::CreateAppendEntryPayload(body_builder, body_builder.CreateVector(log_entries));
    body_builder.Finish(payload);
    // set the frame
    uint8_t *header_buffer = header_builder.GetBufferPointer();
    uint8_t *body_buffer = body_builder.GetBufferPointer();
    int header_size = header_builder.GetSize();
    int body_size = body_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), header_buffer,
                               header_buffer + header_size);
    ret_frame->m_data.insert(ret_frame->m_data.end(), body_buffer, body_buffer + body_size);
    ret_frame->flag_request();
    return ret_frame;
}

std::unique_ptr<client::append_entry_reply> client::append_entry_reply::from_frame(
    transport::frame &frame) {
    std::string buffer_header = frame.m_header;
    std::cout << "buffer_header.size: " << buffer_header.size() << std::endl;
    auto header = flatbuffers::GetRoot<karma_rpc::AppendEntryReply>(buffer_header.data());
    // raft::append_reply reply{.current_term = header->term(), .commit_idx =
    // header->index(), }
    raft::append_reply reply{.current_term = static_cast<raft::term_t>(header->term()),
                             .commit_idx = static_cast<raft::index_t>(header->index())};
    if (header->result_type() == karma_rpc::AppendEntryResult_AppendEntryAccepted) {
        raft::append_reply::accepted accepted{
            .last_new_idx = static_cast<raft::index_t>(
                header->result_as_AppendEntryAccepted()->last_new_idx())};
        reply.result = accepted;
    } else if (header->result_type() == karma_rpc::AppendEntryResult_AppendEntryRejected) {
        raft::append_reply::rejected rejected{
            .non_matching_idx = static_cast<raft::index_t>(
                header->result_as_AppendEntryRejected()->non_matching_idx()),
            .last_idx =
                static_cast<raft::index_t>(header->result_as_AppendEntryRejected()->last_idx())};
        reply.result = rejected;
    }
    return std::make_unique<append_entry_reply>(header->from_id(), header->group_id(), reply);
}

std::unique_ptr<transport::frame> client::append_entry_reply::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_APPEND_ENTRY);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    if (m_reply.result.index() == 1) {
        auto result = std::get<raft::append_reply::accepted>(m_reply.result);
        auto accepted = karma_rpc::CreateAppendEntryAccepted(header_builder, result.last_new_idx);
        auto header = karma_rpc::CreateAppendEntryReply(
            header_builder, m_from_id, m_group_id, m_reply.current_term, m_reply.commit_idx,
            karma_rpc::AppendEntryResult_AppendEntryAccepted, accepted.Union());
        header_builder.Finish(header);
    } else {
        auto result = std::get<raft::append_reply::rejected>(m_reply.result);
        auto rejected = karma_rpc::CreateAppendEntryRejected(
            header_builder, result.non_matching_idx, result.last_idx);
        auto header = karma_rpc::CreateAppendEntryReply(
            header_builder, m_from_id, m_group_id, m_reply.current_term, m_reply.commit_idx,
            karma_rpc::AppendEntryResult_AppendEntryRejected, rejected.Union());
        header_builder.Finish(header);
    }
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_response();
    return ret_frame;
}
