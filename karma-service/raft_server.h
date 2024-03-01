#pragma once
#include <flatbuffers/flatbuffer_builder.h>

#include <boost/log/trivial.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "co_context/shared_task.hpp"
#include "co_context/task.hpp"
#include "karma-raft/fsm.hh"
#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
namespace service {
class raft_server {
   public:
    raft_server(raft::server_id id, std::unique_ptr<raft::state_machine> sm,
                std::unique_ptr<raft::rpc> rpc_, std::unique_ptr<raft::failure_detector> fd_,
                std::vector<raft::config_member> members);
    co_context::task<> start();
    void tick() { _fsm->tick(); }
    bool is_leader() { return _fsm->is_leader(); }
    static std::unique_ptr<raft_server> create(raft::server_id id,
                                               std::unique_ptr<raft::state_machine> sm,
                                               std::unique_ptr<raft::rpc> rpc_,
                                               std::unique_ptr<raft::failure_detector> fd_,
                                               std::vector<raft::config_member> members) {
        return std::make_unique<raft_server>(id, std::move(sm), std::move(rpc_), std::move(fd_),
                                             members);
    }

    void receive(raft::server_id from, raft::rpc_message msg);
    co_context::task<> wait_for_commit(raft::index_t idx);
    co_context::task<> wait_for_apply(raft::index_t idx);
    template <typename Message>
    co_context::task<> send_message(raft::server_id id, Message msg);
    co_context::task<> io_fiber();
    raft::index_t apply_index() { return _apply_idx; }
    co_context::task<> add_entry(raft::command command) {
        auto entry = _fsm->add_entry(command);
        co_await wait_for_commit(entry.idx);
    }
    co_context::task<> read_barrier() {
        auto read_idx = _fsm->start_read_barrier(_id)->second;
        co_await wait_for_apply(read_idx);
    }
    co_context::task<std::string> cli_read(std::string key);
    co_context::task<> cli_write(std::string key, std::string value);

   private:
    struct waiter {
        raft::index_t idx;
        co_context::channel<std::monostate>* promise;
    };
    raft::server_id _id;
    std::unique_ptr<raft::fsm> _fsm;
    std::vector<waiter> _commit_waiters;
    std::vector<waiter> _apply_waiters;

    // persistance
    raft::term_t _term;
    raft::server_id _vote_id;
    raft::index_t _commit_idx;
    std::vector<raft::log_entry_ptr> _entries;

    // state machine
    // std::map<std::string, std::string> _values;
    std::unique_ptr<raft::state_machine> _state_machine;

    // rpc
    std::unique_ptr<raft::rpc> _rpc;

    std::unique_ptr<raft::failure_detector> _failure_detector;

    raft::index_t _apply_idx = 0;

    //
    std::vector<raft::config_member> _members;
};
}  // namespace service
