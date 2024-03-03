#pragma once
#include <memory>
#include <optional>

#include "co_context/net/socket.hpp"
#include "session.h"
namespace client {
class composite_session {
   public:
    static co_context::task<std::optional<std::unique_ptr<composite_session>>>
    new_composite_session(std::string host, uint16_t port);
    composite_session(std::unique_ptr<session> sess) : m_session(std::move(sess)){};
    co_context::task<bool> request(client::task& task);
    bool valid() { return m_session->valid(); }

   private:
    // TODO: make it a group of sessions
    std::unique_ptr<session> m_session;
    session& pick_session() { return *m_session; }
};
};  // namespace client