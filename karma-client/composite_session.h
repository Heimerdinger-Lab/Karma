#pragma once
#include <memory>
#include <optional>

#include "co_context/net/socket.hpp"
#include "session.h"
namespace client {
class composite_session {
   public:
    static co_context::task<std::optional<std::unique_ptr<composite_session>>>
    new_composite_session(std::string host, uint16_t port) {
        co_context::inet_address addr;
        if (co_context::inet_address::resolve(host, port, addr)) {
            // co_context::socket sock{co_context::socket::create_tcp(addr.family())};
            auto sock =
                std::make_unique<co_context::socket>(co_context::socket::create_tcp(addr.family()));
            // 连接一个 server
            int res = co_await sock->connect(addr);
            if (res < 0) {
                printf("res=%d: %s\n", res, strerror(-res));
                co_return std::nullopt;
            }
            auto conn = std::make_unique<transport::connection>(std::move(sock), host, port);
            auto sess = std::make_unique<session>(std::move(conn));
            co_return std::make_unique<composite_session>(std::move(sess));
        } else {
            printf("Unable to resolve %s\n", host.data());
        }
        co_return std::nullopt;
    };
    composite_session(std::unique_ptr<session> sess) : m_session(std::move(sess)){};

    co_context::task<void> request(client::task& task) {
        auto session = std::move(pick_session());
        co_await session.write(task);
    }

   private:
    // 应该是一组session
    std::unique_ptr<session> m_session;
    session& pick_session() { return *m_session; }
};
};  // namespace client