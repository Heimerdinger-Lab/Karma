#include "session_manager.h"

#include <boost/log/trivial.hpp>
co_context::task<std::optional<std::reference_wrapper<client::composite_session>>>
client::session_manager::get_composite_session(std::string host, uint16_t port) {
    clean_dead_session();
    co_context::inet_address addr;
    if (co_context::inet_address::resolve(host, port, addr)) {
        if (m_sessions.find(addr.to_ip_port()) == m_sessions.end()) {
            auto cs = co_await composite_session::new_composite_session(host, port);
            if (cs.has_value()) {
                m_sessions[addr.to_ip_port()] = std::move(cs.value());
            } else {
                BOOST_LOG_TRIVIAL(error)
                    << "Fail to create a composite session to " << addr.to_ip_port();
                co_return std::nullopt;
            }
        }
        co_return *m_sessions[addr.to_ip_port()];
    };
    co_return std::nullopt;
}

void client::session_manager::clean_dead_session() {
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        if (!it->second->valid()) {
            BOOST_LOG_TRIVIAL(trace) << "Remove a session: " << it->first;
            it = m_sessions.erase(it);
        } else {
            it++;
        }
    }
}
