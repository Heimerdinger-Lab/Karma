#pragma once
#include <co_context/all.hpp>
#include <memory>

#include "frame.h"
namespace transport {
struct write_task {
    std::shared_ptr<frame> m_frame;
    std::shared_ptr<co_context::channel<std::optional<connection_error>, 0>> m_observer;
};
}  // namespace transport