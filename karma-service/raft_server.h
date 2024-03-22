#pragma once
#include <flatbuffers/flatbuffer_builder.h>

#include <boost/log/trivial.hpp>
#include <memory>
#include <variant>
#include <vector>

#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "co_context/shared_task.hpp"
#include "co_context/task.hpp"
#include "karma-raft/fsm.hh"
#include "karma-raft/raft.hh"
#include "karma-service/raft/raft_failure_detector.h"
#include "karma-service/raft/raft_rpc.h"
#include "karma-service/raft/raft_state_machine.h"
#include "karma-service/raft/temp_persistence.h"
#include "protocol/rpc_generated.h"
#include "raft/raft_persistence.h"
namespace service {
class raft_server {
   public:
    raft_server(raft::server_id id, std::unique_ptr<service::raft_state_machine> sm,
                std::unique_ptr<service::raft_rpc> rpc_,
                std::unique_ptr<raft::failure_detector> fd_,
                std::unique_ptr<service::temp_persistence> persistence_,
                std::vector<raft::config_member> members);
    co_context::task<> start();
    void tick() { _fsm->tick(); }
    bool is_leader() { return _fsm->is_leader(); }
    static std::unique_ptr<raft_server> create(
        raft::server_id id, std::unique_ptr<service::raft_state_machine> sm,
        std::unique_ptr<service::raft_rpc> rpc_,
        std::unique_ptr<service::raft_failure_detector> fd_,
        std::unique_ptr<service::temp_persistence> persistence_,
        std::vector<raft::config_member> members) {
        return std::make_unique<raft_server>(id, std::move(sm), std::move(rpc_), std::move(fd_),
                                             std::move(persistence_), members);
    }

    void receive(raft::server_id from, raft::rpc_message msg);
    co_context::task<> wait_for_commit(raft::index_t idx);
    co_context::task<> wait_for_apply(raft::index_t idx);
    template <typename Message>
    co_context::task<> send_message(raft::server_id id, Message msg);
    co_context::task<> io_fiber(raft::index_t last_stable);
    co_context::task<> apply_fiber();
    raft::index_t apply_index() { return _apply_idx; }

    // session 直接调它
    co_context::task<bool> add_entry(raft::command command) {
        // 执行add_entry，如果当前是leader，则直接执行，否则转发给leader
        // auto leader_id = _fsm->current_leader();
        // if (leader_id == raft::server_id{}) {
        //     co_return false;
        // }
        // if (leader_id == _id) {
        //     // 我就是女王，自信放光芒
        //     auto& entry = _fsm->add_entry(command);
        //     co_await wait_for_commit(entry.idx);
        // } else {
        //     // 呜呜呜我不是leader
        //     auto reply = co_await _rpc->forward_add_entry(leader_id, command);
        //     co_await wait_for_commit();
        // }
    }
    // 注意：这里返回的read idx是此时leader的commit idx
    // FSM中用的read idx是另一个东西
    co_context::task<raft::read_barrier_reply> get_read_idx() {
        // TODO: forward to leader by two-way rpc
        auto leader_id = _fsm->current_leader();
        if (leader_id == raft::server_id{}) {
            co_return false;
        }
        if (leader_id == _id) {
            auto rid = _fsm->start_read_barrier(_id);
            auto channel = std::make_unique<co_context::channel<std::monostate>>();
            _active_reads.push_back(active_read{
                .id = rid->first,
                .idx = rid->second,
                .promise = channel.get(),
            });
            co_await channel->acquire();
            co_return rid->second;
        } else {
            co_return co_await _rpc->forward_read_barrier(leader_id);
        };
    }
    co_context::task<bool> read_barrier() {
        auto res = co_await get_read_idx();
        auto read_idx = std::get<raft::index_t>(res);
        co_await wait_for_apply(read_idx);
        co_return true;
    }
    co_context::task<std::optional<std::string>> cli_read(std::string key);
    co_context::task<bool> cli_write(std::string key, std::string value);

   private:
    struct waiter {
        raft::index_t idx;
        co_context::channel<std::monostate>* promise;
    };
    struct active_read {
        raft::read_id id;
        raft::index_t idx;
        co_context::channel<std::monostate>* promise;
    };
    raft::server_id _id;
    std::unique_ptr<raft::fsm> _fsm;
    std::vector<waiter> _commit_waiters;
    std::vector<waiter> _apply_waiters;
    std::vector<active_read> _active_reads;
    // persistance
    // std::unique_ptr<service::raft_persistence> _persistence;
    std::unique_ptr<service::temp_persistence> _persistence;
    std::unique_ptr<service::raft_state_machine> _state_machine;
    std::unique_ptr<service::raft_rpc> _rpc;
    std::unique_ptr<raft::failure_detector> _failure_detector;

    // 内存中的apply index
    // commit index 在FSM中
    raft::index_t _apply_idx = 0;

    std::vector<raft::config_member> _members;

    // io_fiber和applier_fiber通信的channel
    co_context::channel<std::vector<raft::log_entry_ptr>, 1024> _apply_entries;
};
}  // namespace service
