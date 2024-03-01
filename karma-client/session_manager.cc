#include "session_manager.h"
co_context::task<std::optional<std::reference_wrapper<client::composite_session>>>
client::session_manager::get_composite_session(std::string host, uint16_t port) {
    //
    std::cout << "get_composite_session: " << host << ":" << port << std::endl;
    co_context::inet_address addr;
    if (co_context::inet_address::resolve(host, port, addr)) {
        if (m_sessions.find(addr.to_ip_port()) == m_sessions.end()) {
            auto cs = co_await composite_session::new_composite_session(host, port);
            if (cs != std::nullopt) {
                m_sessions[addr.to_ip_port()] = std::move(cs.value());
            } else {
                co_return std::nullopt;
            }
        }
        co_return *m_sessions[addr.to_ip_port()];
    };
    co_return std::nullopt;
}
