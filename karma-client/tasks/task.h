#pragma once
#include "co_context/co/channel.hpp"
#include "co_context/task.hpp"

// #include "karma-raft/raft.hh"
// #include "karma-raft/common.h"
// #include "karma-raft/rpc_message.h"
#include <flatbuffers/buffer.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <variant>

#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
namespace client {
class task {
   public:
    virtual std::unique_ptr<transport::frame> gen_frame() = 0;
    virtual co_context::task<void> callback(transport::frame& reply_frame) = 0;
};
};  // namespace client
