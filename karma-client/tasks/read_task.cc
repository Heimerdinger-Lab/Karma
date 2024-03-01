#include "read_task.h"
std::unique_ptr<client::read_request> client::read_request::from_frame(transport::frame &frame) {
    std::string buffer_header = frame.m_header;
    auto header = flatbuffers::GetRoot<karma_rpc::ReadRequest>(buffer_header.data());
    return std::make_unique<read_request>(header->group_id(), header->key()->str());
};

std::unique_ptr<transport::frame> client::read_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_TASK);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto msg = header_builder.CreateString(m_key);
    auto header = karma_rpc::CreateReadReply(header_builder, m_group_id, msg);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_request();
    return ret_frame;
}

std::unique_ptr<client::read_reply> client::read_reply::from_frame(transport::frame &frame) {
    auto header = flatbuffers::GetRoot<karma_rpc::ReadReply>(frame.m_header.data());
    return std::make_unique<read_reply>(header->success(), header->value()->str());
};

std::unique_ptr<transport::frame> client::read_reply::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_READ_TASK);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto msg = header_builder.CreateString(m_value);
    auto header = karma_rpc::CreateReadReply(header_builder, m_success, msg);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_response();
    return ret_frame;
};
