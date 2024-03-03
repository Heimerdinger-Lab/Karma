#include "read_quorum_task.h"

#include <boost/log/trivial.hpp>
std::unique_ptr<transport::frame> client::read_quorum_reply::gen_frame() {
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

std::unique_ptr<client::read_quorum_reply> client::read_quorum_reply::from_frame(
    transport::frame &frame) {
    auto header = flatbuffers::GetRoot<karma_rpc::ReadQuorumReply>(frame.m_header.data());
    raft::read_quorum_reply reply{.current_term = static_cast<raft::term_t>(header->current_term()),
                                  .commit_idx = static_cast<raft::index_t>(header->commit_idx()),
                                  .id = static_cast<raft::read_id>(header->id())};
    BOOST_LOG_TRIVIAL(trace) << "Generate an read quorum reply from frame";
    return std::make_unique<read_quorum_reply>(header->from_id(), header->group_id(), reply);
};

std::unique_ptr<transport::frame> client::read_quorum_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_QUORUM);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header =
        karma_rpc::CreateReadQuorum(header_builder, m_from_id, m_group_id, m_request.current_term,
                                    m_request.leader_commit_idx, m_request.id);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_request();
    return ret_frame;
}

std::unique_ptr<client::read_quorum_request> client::read_quorum_request::from_frame(
    transport::frame &frame) {
    auto header = flatbuffers::GetRoot<karma_rpc::ReadQuorum>(frame.m_header.data());
    raft::read_quorum request{
        .current_term = static_cast<raft::term_t>(header->current_term()),
        .leader_commit_idx = static_cast<raft::index_t>(header->leader_commit_idx()),
        .id = static_cast<raft::read_id>(header->id())};
    BOOST_LOG_TRIVIAL(trace) << "Generate an read quorum reply from frame";
    return std::make_unique<read_quorum_request>(header->from_id(), header->group_id(), request);
};
