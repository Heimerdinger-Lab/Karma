#pragma once
#include <stdint.h>
#include <chrono>
namespace raft {
class logical_clock final {
public:
    using rep = int64_t;
    // There is no realistic period for a logical clock,
    // just use the smallest period possible.
    using period = std::chrono::nanoseconds::period;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<logical_clock, duration>;

    void advance(duration diff = duration{1}) {
        m_now += diff;
    }
    time_point now() const noexcept {
        return m_now;
    }
    static constexpr time_point min() {
        return time_point(duration{0});
    };
private:
    time_point m_now = min();
};
}
