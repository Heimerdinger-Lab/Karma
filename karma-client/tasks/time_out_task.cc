#include "time_out_task.h"
std::unique_ptr<client::time_out_request> client::time_out_request::from_frame(
    transport::frame& frame) {
    std::string buffer_header = frame.m_header;
    auto header = flatbuffers::GetRoot<karma_rpc::TimeOut>(buffer_header.data());
    raft::timeout_now request{.current_term = static_cast<raft::term_t>(header->current_term())};
    return std::make_unique<time_out_request>(header->from_id(), header->group_id(), request);
};

std::unique_ptr<transport::frame> client::time_out_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_TIME_OUT);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header =
        karma_rpc::CreateTimeOut(header_builder, m_from_id, m_group_id, m_request.current_term);
    header_builder.Finish(header);
    // set the frame
    uint8_t* buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_request();
    return ret_frame;
}
