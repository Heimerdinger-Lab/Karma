#include "echo_task.h"
std::unique_ptr<client::echo_request> client::echo_request::from_frame(transport::frame& frame) {
    std::string buffer_header = frame.m_header;
    std::cout << "buffer_header.size: " << buffer_header.size() << std::endl;
    auto header = flatbuffers::GetRoot<karma_rpc::EchoRequest>(buffer_header.data());
    std::cout << "from_frame, header: " << header->msg()->str() << std::endl;
    return std::make_unique<echo_request>(header->from_id(), header->group_id(),
                                          header->msg()->str());
    // return std::make_shared<echo_request>(0, 0, "123");
};

std::unique_ptr<transport::frame> client::echo_request::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto msg = header_builder.CreateString(m_msg);
    auto header = karma_rpc::CreateEchoRequest(header_builder, m_from_id, m_group_id, msg);
    header_builder.Finish(header);
    // set the frame
    uint8_t* buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_request();
    return ret_frame;
};

std::unique_ptr<client::echo_reply> client::echo_reply::from_frame(transport::frame& frame) {
    std::string buffer_header = frame.m_header;
    auto header = flatbuffers::GetRoot<karma_rpc::EchoReply>(buffer_header.data());
    std::cout << "from_frame, header: " << header->msg()->str() << std::endl;
    return std::make_unique<echo_reply>(header->from_id(), header->group_id(),
                                        header->msg()->str());
};

std::unique_ptr<transport::frame> client::echo_reply::gen_frame() {
    auto ret_frame =
        std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
    // header
    flatbuffers::FlatBufferBuilder header_builder;
    auto msg = header_builder.CreateString(m_msg);
    auto header = karma_rpc::CreateEchoReply(header_builder, m_from_id, m_group_id, msg);
    header_builder.Finish(header);
    // set the frame
    uint8_t* buffer = header_builder.GetBufferPointer();
    int size = header_builder.GetSize();
    ret_frame->m_header.insert(ret_frame->m_header.end(), buffer, buffer + size);
    ret_frame->flag_response();
    return ret_frame;
};
