#include "frame.h"
#include <co_context/all.hpp>
namespace transport {
class write_task {
public:
    frame m_frame;
    co_context::condition_variable m_cv;
};
}