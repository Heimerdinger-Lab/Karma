#pragma once
#include <co_context/all.hpp>
#include <memory>

#include "frame.h"
namespace transport {
struct write_task {
    frame& m_frame;
    co_context::channel<std::optional<connection_error>>& m_observer;
};
}  // namespace transport