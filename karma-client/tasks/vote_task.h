#include "protocol/rpc_generated.h"
#include "task.h"
#include <cstdint>
#include <variant>
#include <vector>

namespace client {

class vote_request : public task {
public:
    std::shared_ptr<transport::frame> gen_frame() override {
        
    }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {

    }
};
class vote_reply : public task {
public:
    std::shared_ptr<transport::frame> gen_frame() override {
        
    }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {

    }
};
}