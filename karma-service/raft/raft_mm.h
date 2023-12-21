#pragma once

#include "karma-raft/common.h"
#include "karma-raft/server.h"
#include "raft/raft_rpc.h"
#include "raft/raft_state_machine.h"
#include <memory>
class raft_mm {
public:
    raft_mm() {
        // spawn出来一个协程，去不断polling任务队列
    }
    // 这是把rpc包装了一下，供client全局跨线程调用
    void append_entry() {

    }
private:
    // 对外面只需要提供rpc和state machine就行
    // persistence在server里面，外面不用管
    // 而
    std::unique_ptr<raft::server> m_server;
    raft_state_machine* m_state_machine;
    raft_rpc* rpc;
};