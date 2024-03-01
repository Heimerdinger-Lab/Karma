#include "vote_task.h"
std::unique_ptr<transport::frame> client::vote_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_VOTE);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header =
        karma_rpc::CreateVoteRequest(header_builder, m_from_id, m_group_id, m_request.current_term,
                                     m_request.last_log_idx, m_request.last_log_term);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_request();
    return ret_frame;
}

std::unique_ptr<client::vote_request> client::vote_request::from_frame(transport::frame &frame) {
    std::string buffer_header = frame.m_header;
    auto header = flatbuffers::GetRoot<karma_rpc::VoteRequest>(buffer_header.data());
    raft::vote_request request{.current_term = static_cast<raft::term_t>(header->current_term()),
                               .last_log_idx = static_cast<raft::index_t>(header->last_log_idx()),
                               .last_log_term = static_cast<raft::term_t>(header->last_log_term()),
                               .is_prevote = false,
                               .force = false};
    return std::make_unique<vote_request>(header->from_id(), header->group_id(), request);
};

std::unique_ptr<transport::frame> client::vote_reply::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_VOTE);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header = karma_rpc::CreateVoteReply(header_builder, m_from_id, m_group_id,
                                             m_reply.current_term, m_reply.vote_granted);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_response();
    return ret_frame;
}

std::unique_ptr<client::vote_reply> client::vote_reply::from_frame(transport::frame &frame) {
    std::string buffer_header = frame.m_header;
    auto header = flatbuffers::GetRoot<karma_rpc::VoteReply>(buffer_header.data());
    raft::vote_reply reply{.current_term = static_cast<raft::term_t>(header->current_term()),
                           .vote_granted = static_cast<bool>(header->vote_granted()),
                           .is_prevote = false};
    return std::make_unique<vote_reply>(header->from_id(), header->group_id(), reply);
};
