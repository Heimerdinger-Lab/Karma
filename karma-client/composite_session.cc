#include "composite_session.h"
co_context::task<std::optional<std::unique_ptr<client::composite_session>>>
client::composite_session::new_composite_session(std::string host, uint16_t port) {
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

co_context::task<void> client::composite_session::request(client::task& task) {
    auto session = std::move(pick_session());
    co_await session.write(task);
}
