#include "session.h"
co_context::task<void> client::session::write(task& task_) {
    auto f = task_.gen_frame();
    m_inflight_requests[f->m_seq] = &task_;
    auto result = co_await m_connection->write_frame(*f);
    if (result.has_value()) {
        std::cout << "result: " << result.value() << std::endl;
    }
};

co_context::task<void> client::session::loop() {
    while (1) {
        auto f = co_await m_connection->read_frame();
        if (f->is_response()) {
            std::cout << "response!!!!" << std::endl;
            auto seq = f->m_seq;
            auto task = m_inflight_requests[seq];
            if (f->m_operation_code == karma_rpc::OperationCode_ECHO) {
                auto s = (echo_request&)(*task);
                co_context::co_spawn(s.callback(*f));
            } else if (f->m_operation_code == karma_rpc::OperationCode_READ_TASK) {
                auto s = (read_request&)(*task);
                co_context::co_spawn(s.callback(*f));
            } else if (f->m_operation_code == karma_rpc::OperationCode_WRITE_TASK) {
                auto s = (write_request&)(*task);
                // co_context::co_spawn(s->callback(f));
                co_await s.callback(*f);
            }
        }
    }
};
