#pragma once
// 一个线程一个worker 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "co_context/net/socket.hpp"
#include "co_context/task.hpp"
#include "karma-client/client.h"
#include "karma-service/config.h"
#include "karma-service/raft/raft_failure_detector.h"
#include "karma-service/raft/raft_rpc.h"
#include "karma-service/raft/raft_state_machine.h"
#include "karma-service/session.h"
#include "karma-raft/simple_server.hh"
#include "karma-raft/raft.hh"
#include <cstdint>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <vector>
namespace service {
class worker {
public:
    worker(config cfg)
        : m_cfg(cfg) {
    };
    void start() {
        m_client = std::make_shared<client::client>(m_cfg.m_members);
        auto rpc_ = std::make_unique<service::raft_rpc>(m_client);
        auto sm_ = std::make_unique<service::raft_state_machine>();
        auto fd_ = std::make_unique<service::raft_failure_detector>();
        std::vector<raft::config_member> members;
        for (int i = 0; i < m_cfg.m_count; i++) {
            raft::server_address address(i, m_cfg.m_members[i]);
            raft::config_member member(address, true);
            members.push_back(member);
        }
        m_raft_service = sb_server::create(m_cfg.m_id, std::move(sm_), std::move(rpc_), std::move(fd_), members);
    }
    co_context::task<> loop() {
        while (true) {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            auto sockfd = co_await m_ac->accept((sockaddr*)&addr, &addr_len);
            std::cout << "receive connection: " << inet_ntoa(addr.sin_addr) << ", " << addr.sin_port << std::endl;
            auto sock = std::make_shared<co_context::socket>(sockfd);
            auto session = std::make_shared<service::session>(sock, "127.0.0.1", addr.sin_port);
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
}
