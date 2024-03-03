#include "cli_write_task.h"

#include <boost/log/trivial.hpp>
std::unique_ptr<client::cli_write_request> client::cli_write_request::from_frame(
    transport::frame &frame) {
    auto header = flatbuffers::GetRoot<karma_rpc::WriteRequest>(frame.m_header.data());
    BOOST_LOG_TRIVIAL(trace) << "Generate a client write request from frame";
    return std::make_unique<cli_write_request>(header->group_id(), header->key()->str(),
                                               header->value()->str());
};

std::unique_ptr<transport::frame> client::cli_write_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_WRITE_TASK);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto key = header_builder.CreateString(m_key);
    auto value = header_builder.CreateString(m_value);
    auto header = karma_rpc::CreateWriteRequest(header_builder, m_group_id, key, value);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_request();
    return ret_frame;
}

std::unique_ptr<client::cli_write_reply> client::cli_write_reply::from_frame(
    transport::frame &frame) {
    auto header = flatbuffers::GetRoot<karma_rpc::WriteReply>(frame.m_header.data());
    BOOST_LOG_TRIVIAL(trace) << "Generate a client write reply from frame";
    return std::make_unique<cli_write_reply>(header->success());
};

std::unique_ptr<transport::frame> client::cli_write_reply::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_WRITE_TASK);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto header = karma_rpc::CreateWriteReply(header_builder, m_success);
    header_builder.Finish(header);
    // set the frame
    uint8_t *buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_response();
    return ret_frame;
};
