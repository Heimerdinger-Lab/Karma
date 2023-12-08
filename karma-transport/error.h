#pragma once 
#include <exception>
namespace transport {
enum FrameError {
    Incomplete,
    BadFrame,
    ConnectionReset,
};
class frame_error final : public std::exception {
    FrameError m_error;
public:
    frame_error(FrameError error) : m_error(error) {}
    static frame_error incomplete() {
        return frame_error(FrameError::Incomplete);
    };
    static frame_error bad_frame() {
        return frame_error(FrameError::BadFrame);
    };
    static frame_error connection_reset() {
        return frame_error(FrameError::ConnectionReset);
    };
    bool is_incomplete() {
        return m_error == FrameError::Incomplete;
    }
};
};