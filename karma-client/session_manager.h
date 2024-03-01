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
    std::map<std::string, std::unique_ptr<composite_session>> m_sessions;
};
}  // namespace client