#include "protocol/rpc_generated.h"
#include "task.h"
#include <cstdint>
#include <variant>
#include <vector>

namespace client {
class time_out_request : public task {
    std::shared_ptr<transport::frame> gen_frame() override {
        
    }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {

    }
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_current_term;
};
}