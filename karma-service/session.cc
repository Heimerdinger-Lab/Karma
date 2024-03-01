#include "session.h"
co_context::task<void> service::session::read_loop(
    co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
    transport::connection& conn) {
    while (true) {
        std::cout << "????" << std::endl;
        std::unique_ptr<transport::frame> f;
        try {
            f = co_await conn.read_frame();
        } catch (transport::frame_error e) {
            std::cout << "frame error" << std::endl;
        }
        // printf("f.header = %p, count = %ld\n", f->m_header.data(), f.use_count());
        // std::cout << "!!!!: " << f->m_header.size() <<  std::endl;
        if (f->is_request()) {
            if (f->m_operation_code == karma_rpc::OperationCode_ECHO) {
                // co_context::co_spawn([f, channel]() -> co_context::task<> {
                //     auto echo_req = client::echo_request::from_frame(f);
                //     std::cout << "echo_req.msg: " << echo_req->msg() << std::endl;
                //     client::echo_reply reply(0, 0, "reply from tianpingan");
                //     co_await channel->release(reply.gen_frame());
                //     co_return;
                // }());
                {
                    auto echo_req = client::echo_request::from_frame(*f);
                    std::cout << "echo_req.msg: " << echo_req->msg() << std::endl;
                    client::echo_reply reply(0, 0,
                                             "reply from tianpingan of (" + echo_req->msg() + ").");
                    co_await channel.release(reply.gen_frame());
                    // co_return;
                }
            } else if (f->m_operation_code == karma_rpc::OperationCode_VOTE) {
                std::cout << "vote request" << std::endl;
                auto vote_request = client::vote_request::from_frame(*f);
                m_raft.receive(vote_request->from_id(), vote_request->request());
            } else if (f->m_operation_code == karma_rpc::OperationCode_HEARTBEAT) {
                std::cout << "heartbeat" << std::endl;
                // auto vote_request = client::vote_request::from_frame(f);
                // co_await m_raft.receive(vote_request->from_id(),
                // vote_request->request());
            } else if (f->m_operation_code == karma_rpc::OperationCode_APPEND_ENTRY) {
                std::cout << "append entry" << std::endl;
                auto append_request = client::append_entry_request::from_frame(*f);
                m_raft.receive(append_request->from_id(), append_request->request());
            } else if (f->m_operation_code == karma_rpc::OperationCode_READ_QUORUM) {
                std::cout << "read quorum" << std::endl;
                auto read_quorum_request = client::read_quorum_request::from_frame(*f);
                m_raft.receive(read_quorum_request->from_id(), read_quorum_request->request());
            } else if (f->m_operation_code == karma_rpc::OperationCode_READ_TASK) {
                auto read_task = client::read_request::from_frame(*f);
                std::cout << "read request!!!, key: " << read_task->key() << std::endl;
                auto val = co_await m_raft.cli_read(read_task->key());
                client::read_reply reply(true, val);
                auto frame = std::move(reply.gen_frame());
                frame->m_seq = f->m_seq;
                co_await channel.release(std::move(frame));
            } else if (f->m_operation_code == karma_rpc::OperationCode_WRITE_TASK) {
                auto write_task = client::write_request::from_frame(*f);
                co_await m_raft.cli_write(write_task->key(), write_task->value());
                client::write_reply reply(true);
                auto frame = std::move(reply.gen_frame());
                frame->m_seq = f->m_seq;
                co_await channel.release(std::move(frame));
            } else {
                std::cout << "unknow task" << std::endl;
            }
        } else {
            if (f->m_operation_code == karma_rpc::OperationCode_ECHO) {
                std::cout << "echo reply" << std::endl;
            } else if (f->m_operation_code == karma_rpc::OperationCode_VOTE) {
                std::cout << "vote reply" << std::endl;
                auto vote_reply = client::vote_reply::from_frame(*f);
                m_raft.receive(vote_reply->from_id(), vote_reply->reply());
            } else if (f->m_operation_code == karma_rpc::OperationCode_HEARTBEAT) {
                std::cout << "heartbeat reply" << std::endl;
                // auto vote_request = client::vote_request::from_frame(f);
                // co_await m_raft.receive(vote_request->from_id(),
                // vote_request->request());
            } else if (f->m_operation_code == karma_rpc::OperationCode_APPEND_ENTRY) {
                std::cout << "append entry reply" << std::endl;
                auto append_reply = client::append_entry_reply::from_frame(*f);
                m_raft.receive(append_reply->from_id(), append_reply->reply());
            } else if (f->m_operation_code == karma_rpc::OperationCode_READ_QUORUM) {
                std::cout << "read quorum reply" << std::endl;
                auto read_quorum_request = client::read_quorum_request::from_frame(*f);
                m_raft.receive(read_quorum_request->from_id(), read_quorum_request->request());
            } else {
                std::cout << "unknow reply task" << std::endl;
            }
        }
    }
}

co_context::task<void> service::session::write_loop(
    co_context::channel<std::unique_ptr<transport::frame>, 1024>& channel,
    transport::connection& conn) {
    // response loop
    while (true) {
        auto f = co_await channel.acquire();
        std::cout << "to write a frame" << std::endl;
        // co_context::co_spawn(conn_write(conn, f));
        // co_await conn_write(conn, f);
        co_await conn.write_frame(*f);
    }
}
