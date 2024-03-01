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
    std::unique_ptr<transport::frame> gen_frame() override;
    static std::unique_ptr<time_out_request> from_frame(transport::frame& frame);
    co_context::task<void> callback(transport::frame& reply_frame) override {}

   private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    // uint64_t m_current_term;
    raft::timeout_now m_request;
};
}  // namespace client