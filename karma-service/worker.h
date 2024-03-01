#pragma once
// 一个线程一个worker
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <boost/log/trivial.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "co_context/net/socket.hpp"
#include "co_context/task.hpp"
#include "karma-client/client.h"
#include "karma-raft/raft.hh"
#include "karma-service/config.h"
#include "karma-service/raft/raft_failure_detector.h"
#include "karma-service/raft/raft_rpc.h"
#include "karma-service/raft/raft_state_machine.h"
#include "karma-service/raft_server.h"
#include "karma-service/session.h"
namespace service {
class worker {
   public:
    worker(config cfg) : m_cfg(cfg){};
    co_context::task<> start();
    co_context::task<> loop();

   private:
    config m_cfg;
    std::vector<std::unique_ptr<session>> m_sessions;

    std::unique_ptr<raft_server> m_raft;
    std::unique_ptr<co_context::acceptor> m_ac;
};
}  // namespace service
