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
        BOOST_LOG_TRIVIAL(debug) << "worker starting";
        auto rpc_ = std::make_unique<service::raft_rpc>(
            m_cfg.m_id, std::make_unique<client::client>(m_cfg.m_members));
        auto sm_ = std::make_unique<service::raft_state_machine>();
        auto fd_ = std::make_unique<service::raft_failure_detector>();
        std::vector<raft::config_member> members;
        for (int i = 1; i <= m_cfg.m_count; i++) {
            raft::server_address address(i, m_cfg.m_members[i]);
            raft::config_member member(address, true);
            members.push_back(member);
        }
        m_raft =
            sb_server::create(m_cfg.m_id, std::move(sm_), std::move(rpc_), std::move(fd_), members);

        auto current_address = m_cfg.m_members[m_cfg.m_id];
        size_t pos = current_address.find(':');
        // 提取IP地址和端口号
        std::string ip = current_address.substr(0, pos);
        std::string port = current_address.substr(pos + 1);
        BOOST_LOG_TRIVIAL(debug) << "listenning at " << ip << ":" << port;
        co_context::inet_address addr(ip, std::stoi(port));
        m_ac = std::make_unique<co_context::acceptor>(addr);
        co_await m_raft->start();
        co_context::co_spawn([](sb_server& service) -> co_context::task<> {
            while (true) {
                // std::cout << "tick, is_leader: " << service.is_leader() << std::endl;
                BOOST_LOG_TRIVIAL(debug) << "tick";
                service.tick();
                using namespace std::literals;
                co_await co_context::timeout(1s);
            }
        }(*m_raft));
    }
    co_context::task<> loop() {
        while (true) {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            auto sockfd = co_await m_ac->accept((sockaddr*)&addr, &addr_len);
            BOOST_LOG_TRIVIAL(debug)
                << "receive connection from " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port;
            auto session =
                std::make_unique<service::session>(sockfd, "127.0.0.1", addr.sin_port, *m_raft);
            session->process();
            m_sessions.emplace_back(std::move(session));
        }
    }

   private:
    config m_cfg;
    std::vector<std::unique_ptr<session>> m_sessions;

    std::unique_ptr<sb_server> m_raft;
    std::unique_ptr<co_context::acceptor> m_ac;
};
}  // namespace service
