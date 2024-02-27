#pragma once 
#include "co_context/lazy_io.hpp"
#include "karma-client/tasks/echo_task.h"
#include "karma-client/tasks/append_entry_task.h"
#include "karma-client/tasks/vote_task.h"
#include "karma-client/tasks/time_out_task.h"
#include "karma-client/tasks/read_quorum_task.h"
#include "karma-raft/raft.hh"
#include "karma-client/session_manager.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <sys/socket.h>

namespace client {

class client {
public:
    client(std::map<uint64_t, std::string> members) 
        : m_session_manager (std::make_unique<session_manager>())
        , m_members(members){
    }
    co_context::task<void> rpc_timeout() {
        using namespace std::literals;
        co_await co_context::timeout(3s);   
    }
    co_context::task<std::shared_ptr<echo_reply>> echo(raft::server_address start, raft::server_address target, std::string msg) {
        std::shared_ptr<co_context::channel<std::shared_ptr<echo_reply>>> prom = std::make_shared<co_context::channel<std::shared_ptr<echo_reply>>>();
        std::shared_ptr<echo_request> req = std::make_shared<echo_request>(start.id, 0, msg, prom);
        auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
        
        co_await session.value()->request(req);
        auto reply = co_await prom->acquire();
        co_return reply;
    
    } 
    // co_context::task<> append_entry(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<append_entry_request> req = std::make_shared<append_entry_request>();
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> append_entry_reply_(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<append_entry_reply> req = std::make_shared<append_entry_reply>();
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> vote_request_(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<vote_request> req = std::make_shared<vote_request>(start.id, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> vote_reply_(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<vote_reply> req = std::make_shared<vote_reply>(start.id, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> time_out(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<time_out_request> req = std::make_shared<time_out_request>(start.id, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> read_quorum(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<read_quorum_request> req = std::make_shared<read_quorum_request>(start.id, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
    // co_context::task<> read_quorum_reply_(raft::server_address start, raft::server_address target, std::string msg) {
    //     std::shared_ptr<read_quorum_reply> req = std::make_shared<read_quorum_reply>(start.id, 0, msg);
    //     auto session = co_await m_session_manager->get_composite_session("127.0.0.1", 6666);
    //     if (session.has_value()) {
    //         co_await session.value()->request(req);
    //     }
    // } 
private:
    std::unique_ptr<session_manager> m_session_manager;
    std::map<uint64_t, std::string> m_members;
};
}