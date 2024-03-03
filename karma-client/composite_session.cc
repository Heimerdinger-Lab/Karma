#include "composite_session.h"

#include <boost/log/trivial.hpp>
#include <memory>
co_context::task<std::optional<std::unique_ptr<client::composite_session>>>
client::composite_session::new_composite_session(std::string host, uint16_t port) {
    co_context::inet_address addr;
    if (co_context::inet_address::resolve(host, port, addr)) {
        auto sock =
            std::make_unique<co_context::socket>(co_context::socket::create_tcp(addr.family()));
        int res = co_await sock->connect(addr);
        if (res < 0) {
            BOOST_LOG_TRIVIAL(error) << "Fail to bind socket to the address";
            co_return std::nullopt;
        }
        auto conn = std::make_unique<transport::connection>(std::move(sock), host, port);
        auto sess = std::make_unique<session>(std::move(conn));
        // co_await sess->start();
        BOOST_LOG_TRIVIAL(trace) << "Create an new composite_session to " << host << ":" << port;
        co_return std::make_unique<composite_session>(std::move(sess));
    } else {
        BOOST_LOG_TRIVIAL(error) << "Fail to resolve the address: " << host << ":" << port;
        co_return std::nullopt;
    }
};

co_context::task<bool> client::composite_session::request(client::task& task) {
    auto& session = pick_session();
    // 不会是这里加move的原因吧？？？
    // 傻逼了
    co_return co_await session.write(task);
}
