#pragma once
#include <map>
#include <memory>
#include <optional>

#include "composite_session.h"
namespace client {
class session_manager {
   public:
    co_context::task<std::optional<std::reference_wrapper<composite_session>>>
    get_composite_session(std::string host, uint16_t port);

   private:
    void clean_dead_session();
    std::map<std::string, std::shared_ptr<composite_session>> m_sessions;
};
}  // namespace client