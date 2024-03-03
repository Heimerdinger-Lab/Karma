#include "client.h"
// for test
co_context::task<std::optional<std::unique_ptr<client::cli_echo_reply>>> client::client::cli_echo(
    raft::server_id start, raft::server_id target, std::string msg) {
    auto prom = std::make_unique<co_context::channel<std::unique_ptr<cli_echo_reply>>>();
    auto req = std::make_unique<cli_echo_request>(start, 0, msg);
    req->set_prom(prom.get());
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_await session.value().get().request(*req);
        auto reply = co_await prom->acquire();
        co_return reply;
    }
    co_return std::nullopt;
}

co_context::task<bool> client::client::append_entry(raft::server_id start, raft::server_id target,
                                                    const raft::append_request& append_request) {
    auto req = std::make_unique<append_entry_request>(start, 0, append_request);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

co_context::task<bool> client::client::append_entry_reply_(raft::server_id start,
                                                           raft::server_id target,
                                                           const raft::append_reply& reply) {
    auto req = std::make_unique<append_entry_reply>(start, 0, reply);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

co_context::task<bool> client::client::vote_request_(raft::server_id start, raft::server_id target,
                                                     const raft::vote_request& vote_request_) {
    auto req = std::make_unique<vote_request>(start, 0, vote_request_);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

co_context::task<bool> client::client::vote_reply_(raft::server_id start, raft::server_id target,
                                                   const raft::vote_reply& vote_reply_) {
    auto req = std::make_unique<vote_reply>(start, 0, vote_reply_);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

co_context::task<bool> client::client::time_out(raft::server_id start, raft::server_id target,
                                                const raft::timeout_now& timeout_now_) {
    auto req = std::make_unique<time_out_request>(start, 0, timeout_now_);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

co_context::task<bool> client::client::read_quorum(raft::server_id start, raft::server_id target,
                                                   const raft::read_quorum& read_quorum) {
    auto req = std::make_unique<read_quorum_request>(start, 0, read_quorum);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

co_context::task<bool> client::client::read_quorum_reply_(
    raft::server_id start, raft::server_id target,
    const raft::read_quorum_reply& read_quorum_reply_) {
    auto req = std::make_unique<read_quorum_reply>(start, 0, read_quorum_reply_);
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_return co_await session.value().get().request(*req);
    }
    co_return false;
}

// for client
co_context::task<std::optional<std::unique_ptr<client::cli_read_reply>>> client::client::cli_read(
    raft::server_id group_id, raft::server_id target, std::string key) {
    auto prom = std::make_unique<co_context::channel<std::unique_ptr<cli_read_reply>>>();
    auto req = std::make_unique<cli_read_request>(group_id, key);
    req->set_prom(prom.get());
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);

    if (session.has_value()) {
        co_await session.value().get().request(*req);
        auto reply = co_await prom->acquire();
        co_return reply;
    }
    co_return std::nullopt;
}

co_context::task<std::optional<std::unique_ptr<client::cli_write_reply>>> client::client::cli_write(
    raft::group_id group_id, raft::server_id target, std::string key, std::string value) {
    auto prom = std::make_unique<co_context::channel<std::unique_ptr<cli_write_reply>>>();
    auto req = std::make_unique<cli_write_request>(group_id, key, value);
    req->set_prom(prom.get());
    auto session = co_await m_session_manager->get_composite_session(m_members[target].first,
                                                                     m_members[target].second);
    if (session.has_value()) {
        co_await session.value().get().request(*req);
        auto reply = co_await prom->acquire();
        co_return reply;
    }
    co_return std::nullopt;
}
