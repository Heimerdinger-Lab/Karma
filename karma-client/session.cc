#include "session.h"

#include <boost/log/trivial.hpp>
co_context::task<bool> client::session::write(task& task_) {
    if (!valid()) {
        BOOST_LOG_TRIVIAL(error)
            << "Fail to do a write task on this session because the connection will be reset";
        co_return false;
    }
    auto frame = task_.gen_frame();
    if (!frame->is_request()) {
        BOOST_LOG_TRIVIAL(error)
            << "Write an unexpected frame to this session, the session will be reset";
        co_return false;
    }
    if (m_inflight_requests.contains(frame->m_seq)) {
        BOOST_LOG_TRIVIAL(error)
            << "Fail to do a write task on this session because the wrong sequence of the frame";
        co_return false;
    }
    BOOST_LOG_TRIVIAL(trace) << "An new write task in inflight, the sequence is: " << frame->m_seq;
    m_inflight_requests[frame->m_seq] = &task_;
    co_return co_await m_connection->write_frame(*frame);
};

co_context::task<void> client::session::loop() {
    BOOST_LOG_TRIVIAL(trace) << "Session read loop started";
    while (valid()) {
        auto frame_opt = co_await m_connection->read_frame();
        if (!frame_opt.has_value()) {
            assert(valid() == false);
            BOOST_LOG_TRIVIAL(error)
                << "Fail to do an read task on this session because the connection will be reset";
            co_return;
        }
        auto frame = std::move(frame_opt.value());
        if (frame->is_response()) {
            if (!m_inflight_requests.contains(frame->m_seq)) {
                BOOST_LOG_TRIVIAL(trace)
                    << "Read an unexpected frame from this session, which is not inflight";
                continue;
            }
            BOOST_LOG_TRIVIAL(trace)
                << "Receive an response frame and the sequence is: " << frame->m_seq;

            auto task = m_inflight_requests[frame->m_seq];
            if (frame->m_operation_code == karma_rpc::OperationCode_ECHO) {
                auto s = (cli_echo_request&)(*task);
                BOOST_LOG_TRIVIAL(trace) << "Receive an echo reply";
                co_await s.callback(*frame);
                m_inflight_requests.erase(frame->m_seq);
            } else if (frame->m_operation_code == karma_rpc::OperationCode_READ_TASK) {
                auto s = (cli_read_request&)(*task);
                BOOST_LOG_TRIVIAL(trace) << "Receive an read reply";
                co_await s.callback(*frame);
            } else if (frame->m_operation_code == karma_rpc::OperationCode_WRITE_TASK) {
                auto s = (cli_write_request&)(*task);
                BOOST_LOG_TRIVIAL(trace) << "Receive a write reply";
                co_await s.callback(*frame);
            }
        } else {
            BOOST_LOG_TRIVIAL(error) << "Read an unexpected frame from this session";
            continue;
        }
    }
};
