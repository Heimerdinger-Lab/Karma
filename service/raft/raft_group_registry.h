#pragma once
#include "raft_rpc.h"
#include "rocksdb_persistence.h"

#include "raft/common.h"
#include "raft/raft.h"
#include "raft/server.h"

#include <unordered_map> 
#include <co_context/all.hpp>

struct raft_server_for_group {
    raft::group_id gid;
    std::unique_ptr<raft::server> server;
    // std::unique_ptr<raft_ticker_type> ticker;
    raft_rpc& rpc;
    rocksdb_persistence& persistence;
};

class raft_group_registry {
    std::unordered_map<raft::group_id, raft_server_for_group> m_servers;
public:
    co_context::task<> start();
    raft::server_id& get_my_raft_id();
    raft_rpc& get_rpc(raft::group_id gid);
    raft::server& get_server(raft::group_id gid);
    raft::server* find_server(raft::group_id gid);
    std::vector<raft::group_id> all_groups() const;
    co_context::task<> start_server_for_group(raft_server_for_group grp);
};