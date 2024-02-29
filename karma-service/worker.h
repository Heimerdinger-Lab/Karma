#pragma once
// 一个线程一个worker
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
#include "karma-raft/simple_server.hh"
#include "karma-service/config.h"
#include "karma-service/raft/raft_failure_detector.h"
#include "karma-service/raft/raft_rpc.h"
#include "karma-service/raft/raft_state_machine.h"
#include "karma-service/session.h"
namespace service {
class worker {
   public:
    worker(config cfg) : m_cfg(cfg){};
    co_context::task<> start() {
        // co_context::acceptor ac{co_context::inet_address{port}};
        m_client = std::make_shared<client::client>(m_cfg.m_members);
        auto rpc_ = std::make_unique<service::raft_rpc>(m_cfg.m_id, m_client);
        auto sm_ = std::make_unique<service::raft_state_machine>();
        auto fd_ = std::make_unique<service::raft_failure_detector>();
        std::vector<raft::config_member> members;
        for (int i = 1; i <= m_cfg.m_count; i++) {
            raft::server_address address(i, m_cfg.m_members[i]);
            raft::config_member member(address, true);
            members.push_back(member);
        }
        m_raft_service =
            sb_server::create(m_cfg.m_id, std::move(sm_), std::move(rpc_), std::move(fd_), members);
        // m_ac = co_context::acceptor
        // {co_context::inet_address{m_cfg.m_listen_port}};
        co_context::inet_address addr(m_cfg.m_listen_ip, m_cfg.m_listen_port);
        m_ac = std::make_shared<co_context::acceptor>(addr);
        co_await m_raft_service->start();
        // m_raft_service->tick();
        co_context::co_spawn([](std::shared_ptr<sb_server> service) -> co_context::task<> {
            while (true) {
                std::cout << "tick, is_leader: " << service->is_leader() << std::endl;
                service->tick();
                using namespace std::literals;
                co_await co_context::timeout(1s);
            }
        }(m_raft_service));
    }
    co_context::task<> loop() {
        while (true) {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            auto sockfd = co_await m_ac->accept((sockaddr*)&addr, &addr_len);
            std::cout << "receive connection: " << inet_ntoa(addr.sin_addr) << ", " << addr.sin_port
                      << std::endl;
            auto sock = std::make_shared<co_context::socket>(sockfd);
            auto session = std::make_shared<service::session>(sock, "127.0.0.1", addr.sin_port,
                                                              m_raft_service);
            m_sessions.push_back(session);
            session->process();
        }
    }

   private:
    // uint16_t m_port;
    config m_cfg;
    std::shared_ptr<sb_server> m_raft_service;
    std::shared_ptr<client::client> m_client;
    std::shared_ptr<co_context::acceptor> m_ac;
    std::vector<std::shared_ptr<session>> m_sessions;
};
}  // namespace service
