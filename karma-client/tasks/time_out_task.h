#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {
class time_out_request : public task {
   public:
    time_out_request(uint64_t m_from_id, uint64_t m_group_id, raft::timeout_now m_request)
        : m_from_id(m_from_id), m_group_id(m_group_id), m_request(m_request) {}
    std::unique_ptr<transport::frame> gen_frame() override {
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
    static std::unique_ptr<time_out_request> from_frame(transport::frame& frame) {
        std::string buffer_header = frame.m_header;
        auto header = flatbuffers::GetRoot<karma_rpc::TimeOut>(buffer_header.data());
        raft::timeout_now request{.current_term =
                                      static_cast<raft::term_t>(header->current_term())};
        return std::make_unique<time_out_request>(header->from_id(), header->group_id(), request);
    };
    co_context::task<void> callback(transport::frame& reply_frame) override {}

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    // uint64_t m_current_term;
    raft::timeout_now m_request;
};
}  // namespace client