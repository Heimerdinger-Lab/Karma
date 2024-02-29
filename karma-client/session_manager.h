#pragma once 
#include "composite_session.h"
#include <map>
#include <memory>
#include <optional>
namespace client {
class session_manager {
public:
    co_context::task<std::optional<std::shared_ptr<composite_session>>> get_composite_session(std::string host, uint16_t port) {
        // 
        std::cout << "get_composite_session: " << host << ":" << port << std::endl;  
        co_context::inet_address addr;
        
        if (co_context::inet_address::resolve(host, port, addr)) {
            if (m_sessions.find(addr.to_ip_port()) == m_sessions.end()) {
                auto cs = co_await composite_session::new_composite_session(host, port);
                if (cs != std::nullopt) {
                    m_sessions[addr.to_ip_port()] = cs.value();
                } else {
                    co_return std::nullopt;
                }
            } 
            co_return m_sessions[addr.to_ip_port()];
        };
        co_return std::nullopt;
    }
private:
    std::map<std::string, std::shared_ptr<composite_session>> m_sessions;
};
}