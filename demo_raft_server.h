#pragma once
#include "scylladb-raft/raft.hh"
#include "scylladb-raft/server.hh"
#include <memory>
class demo_server_impl : public raft::rpc_server, public raft::server {
    explicit demo_server_impl(raft::server_id uuid, std::unique_ptr<raft::rpc> rpc,
        std::unique_ptr<raft::state_machine> state_machine, std::unique_ptr<raft::persistence> persistence,
        std::shared_ptr<raft::failure_detector> failure_detector, server::configuration config) {}
};
std::unique_ptr<raft::server> create_demo_server(raft::server_id uuid, std::unique_ptr<raft::rpc> rpc,
    std::unique_ptr<raft::state_machine> state_machine, std::unique_ptr<raft::persistence> persistence,
    std::shared_ptr<raft::failure_detector> failure_detector, raft::server::configuration config) {
    return std::make_unique<demo_server_impl>(uuid, std::move(rpc), std::move(state_machine), std::move(persistence), failure_detector, config);
}