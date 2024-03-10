#include "session.h"

#include "co_context/io_context.hpp"
co_context::task<void> service::session::read_loop(
    co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
    transport::connection& conn) {
    BOOST_LOG_TRIVIAL(trace) << "Session read loop start";
    while (true) {
        auto frame_opt = co_await conn.read_frame();
        if (!frame_opt.has_value()) {
            BOOST_LOG_TRIVIAL(error) << "Session will be reset...";
            co_return;
        }
        BOOST_LOG_TRIVIAL(trace) << "Read a frame from this session";
        auto frame = std::move(frame_opt.value());
        if (frame->is_request()) {
            if (frame->m_operation_code == karma_rpc::OperationCode_ECHO) {
                BOOST_LOG_TRIVIAL(trace) << "Receive an client echo request";
                auto echo_req = client::cli_echo_request::from_frame(*frame);
                client::cli_echo_reply reply(
                    0, 0, "Reply from tianpingan, the request msg is :" + echo_req->msg() + ".");
                auto reply_frame = reply.gen_frame();
                reply_frame->m_seq = frame->m_seq;
                co_await channel.release(std::move(reply_frame));

            } else if (frame->m_operation_code == karma_rpc::OperationCode_VOTE) {
                BOOST_LOG_TRIVIAL(trace) << "Receive an vote request";
                auto vote_request = client::vote_request::from_frame(*frame);
                m_raft.receive(vote_request->from_id(), vote_request->request());
            } else if (frame->m_operation_code == karma_rpc::OperationCode_HEARTBEAT) {
                BOOST_LOG_TRIVIAL(trace) << "Receive a heartbeat";
            } else if (frame->m_operation_code == karma_rpc::OperationCode_APPEND_ENTRY) {
                BOOST_LOG_TRIVIAL(trace) << "Receive an append entry request";
                auto append_request = client::append_entry_request::from_frame(*frame);
                m_raft.receive(append_request->from_id(), append_request->request());
            } else if (frame->m_operation_code == karma_rpc::OperationCode_READ_QUORUM) {
                BOOST_LOG_TRIVIAL(trace) << "Receive an read quorum";
                auto read_quorum_request = client::read_quorum_request::from_frame(*frame);
                m_raft.receive(read_quorum_request->from_id(), read_quorum_request->request());
            } else if (frame->m_operation_code == karma_rpc::OperationCode_READ_TASK) {
                BOOST_LOG_TRIVIAL(trace) << "Receive an client read request";
                auto read_task = client::cli_read_request::from_frame(*frame);
                auto val = co_await m_raft.cli_read(read_task->key());
                if (val.has_value()) {
                    client::cli_read_reply reply(true, val.value());
                    auto frame = reply.gen_frame();
                    frame->m_seq = frame->m_seq;
                    co_await channel.release(std::move(frame));
                } else {
                    client::cli_read_reply reply(false, "");
                    auto frame = reply.gen_frame();
                    frame->m_seq = frame->m_seq;
                    co_await channel.release(std::move(frame));
                }
            } else if (frame->m_operation_code == karma_rpc::OperationCode_WRITE_TASK) {
                BOOST_LOG_TRIVIAL(trace) << "Receive an client write request";
                auto write_task = client::cli_write_request::from_frame(*frame);
                co_await m_raft.cli_write(write_task->key(), write_task->value());
                client::cli_write_reply reply(true);
                auto frame = reply.gen_frame();
                frame->m_seq = frame->m_seq;
                co_await channel.release(std::move(frame));
            } else {
                BOOST_LOG_TRIVIAL(error) << "Receive an unexpected request";
            }
            // 这里还有两种，一种的follower发过来的请求leader去append entry和read的请求
            // 这里去执行execute_add_entry
            // 得到reply后，利用这个session直接发一个response
        } else {
            // 有follower可能收到Append entry 和 read barrier的来自leader的响应
            //
            BOOST_LOG_TRIVIAL(error) << "Receive an unexpected reply on server session";
        }
    }
}

co_context::task<void> service::session::write_loop(
    co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
    transport::connection& conn) {
    BOOST_LOG_TRIVIAL(trace) << "Session write loop start";
    while (true) {
        auto f = std::move(co_await channel.acquire());
        BOOST_LOG_TRIVIAL(trace) << "Session write loop got an write frame task";
        co_await conn.write_frame(*f);
    }
}
