#pragma once 
#include "co_context/net/socket.hpp"
#include "session.h"
#include <memory>
#include <optional>
namespace client {
class composite_session {
public:
    static co_context::task<std::optional<std::shared_ptr<composite_session>>> new_composite_session(std::string host, uint16_t port) {
        co_context::inet_address addr;
        if (co_context::inet_address::resolve(host, port, addr)) {
            // co_context::socket sock{co_context::socket::create_tcp(addr.family())};
            auto sock = std::make_shared<co_context::socket>(co_context::socket::create_tcp(addr.family()));
            // 连接一个 server
            int res = co_await sock->connect(addr);
            if (res < 0) {
                printf("res=%d: %s\n", res, strerror(-res));
                co_return std::nullopt;
            }
            auto conn = std::make_shared<transport::connection>(sock, host, port);
            auto sess = std::make_shared<session>(conn);
            co_return std::make_shared<composite_session>(sess);
        } else {
            printf("Unable to resolve %s\n", host.data());
        }
        co_return std::nullopt;
    };
    composite_session(std::shared_ptr<session> sess)
        : m_session(sess) {};

    co_context::task<void> request(std::shared_ptr<client::task> task) {
        auto session = pick_session();
        co_await session->write(task);
    }
private:
    // 应该是一组session
    std::shared_ptr<session> m_session;
    std::shared_ptr<session> pick_session() {
        return m_session;
    }
};
};