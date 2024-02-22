#pragma once
#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "co_context/task.hpp"
#include "raft.hh"
#include "fsm.hh"
#include <iostream>
#include <map>
#include <memory>
#include <variant>
#include <vector>
class sb_server {
public:
    // 外部
    sb_server(raft::server_id id, std::unique_ptr<raft::state_machine> sm, std::unique_ptr<raft::rpc> rpc_, std::unique_ptr<raft::failure_detector> fd_) 
        : _id(id)
        , _state_machine(std::move(sm))
        , _rpc(std::move(rpc_)) 
        , _failure_detector(std::move(fd_)){
        } 
    co_context::task<> start() {
        raft::snapshot_descriptor snp;
        snp.id = 0;
        snp.idx = 0;
        snp.term = 0;
        raft::config_member member1(raft::server_address(1, "1"), true);
        raft::config_member member2(raft::server_address(2, "2"), true);
        raft::config_member member3(raft::server_address(3, "3"), true);
        snp.config.current.insert(member1);
        snp.config.current.insert(member2);
        snp.config.current.insert(member3);
        auto log = raft::log(snp, std::move(_entries));
        _fsm = std::make_unique<raft::fsm>(_id, _term, _vote_id, log, _commit_idx, *_failure_detector,
                                    raft::fsm_config {
                                        .append_request_threshold = 1024,
                                        .max_log_size = 1024,
                                        .enable_prevoting = false
                                    });
        co_context::co_spawn(io_fiber());
        co_return;
    }
    void tick() {
        _fsm->tick();
    }
    bool is_leader() {
        return _fsm->is_leader();
    }
    static std::unique_ptr<sb_server> create(raft::server_id id, std::unique_ptr<raft::state_machine> sm, std::unique_ptr<raft::rpc> rpc_, std::unique_ptr<raft::failure_detector> fd_) {
        return std::make_unique<sb_server>(id, std::move(sm), std::move(rpc_), std::move(fd_));
    }
     
    co_context::task<> receive(raft::server_id from, raft::rpc_message msg) {
        if (std::holds_alternative<raft::append_request>(msg)) {
            _fsm->step(from, std::move(std::get<raft::append_request>(msg)));
        } else if (std::holds_alternative<raft::append_reply>(msg)) {
            _fsm->step(from, std::move(std::get<raft::append_reply>(msg)));
        } else if (std::holds_alternative<raft::vote_request>(msg)) {
            _fsm->step(from, std::move(std::get<raft::vote_request>(msg)));
        } else if (std::holds_alternative<raft::vote_reply>(msg)) {
            _fsm->step(from, std::move(std::get<raft::vote_reply>(msg)));
        } else if (std::holds_alternative<raft::timeout_now>(msg)) {
            _fsm->step(from, std::move(std::get<raft::timeout_now>(msg)));
        } else if (std::holds_alternative<raft::read_quorum>(msg)) {
            _fsm->step(from, std::move(std::get<raft::read_quorum>(msg)));
        } else if (std::holds_alternative<raft::read_quorum_reply>(msg)) {
            _fsm->step(from, std::move(std::get<raft::read_quorum_reply>(msg)));
        };
        co_return;
    }
    // 内部
    co_context::task<> wait_for_commit(raft::index_t idx) {
        waiter w;
        w.idx = idx;
        w.promise = std::make_shared<co_context::channel<std::monostate>>();
        _commit_waiters.push_back(w);
        co_await w.promise->acquire();
    }
    co_context::task<> wait_for_apply(raft::index_t idx) {
        if (idx >= apply_index()) {
            co_return;
        }
        waiter w;
        w.idx = idx;
        w.promise = std::make_shared<co_context::channel<std::monostate>>();
        _apply_waiters.push_back(w);
        co_await w.promise->acquire();
    }
    template <typename Message>
    co_context::task<> send_message(raft::server_id id, Message msg) {
        if (std::holds_alternative<raft::append_request>(msg)) {
            std::cout << "raft::append_request" << std::endl;
            co_await _rpc->send_append_entries(id, std::get<raft::append_request>(msg));
        } else if (std::holds_alternative<raft::append_reply>(msg)) {
            std::cout << "raft::append_reply" << std::endl;
            _rpc->send_append_entries_reply(id, std::get<raft::append_reply>(msg));
        } else if (std::holds_alternative<raft::vote_request>(msg)) {
            std::cout << "raft::vote_request" << std::endl;
            _rpc->send_vote_request(id, std::get<raft::vote_request>(msg));
        } else if (std::holds_alternative<raft::vote_reply>(msg)) {
            std::cout << "raft::vote_reply" << std::endl;
            _rpc->send_vote_reply(id, std::get<raft::vote_reply>(msg));
        } else if (std::holds_alternative<raft::timeout_now>(msg)) {
            std::cout << "raft::timeout_now" << std::endl;
            _rpc->send_timeout_now(id, std::get<raft::timeout_now>(msg));
        } else if (std::holds_alternative<raft::read_quorum>(msg)) {
            std::cout << "raft::read_quorum" << std::endl;
            _rpc->send_read_quorum(id, std::get<raft::read_quorum>(msg));
        } else if (std::holds_alternative<raft::read_quorum_reply>(msg)) {
            std::cout << "raft::read_quorum_reply" << std::endl;
            _rpc->send_read_quorum_reply(id, std::get<raft::read_quorum_reply>(msg));
        }
        co_return;
    }
    co_context::task<> io_fiber() {
        while (true) {
            std::cout << "io_fiber" << std::endl;
            auto output = co_await _fsm->poll_output();
            if (output.term_and_vote) {
                _term = output.term_and_vote->first;
                _vote_id = output.term_and_vote->second;
            }
            if (output.log_entries.size()) {
                _entries = output.log_entries;
            }
            if (output.committed.size()) {
                // commit and apply
                _commit_idx = output.committed.back()->idx;
                std::vector<raft::command_cref> commands;
                for (auto &item : output.committed) {
                    if (std::holds_alternative<raft::command>(item->data)) {
                        auto cd = std::get<raft::command>(item->data);
                        commands.push_back(cd);
                    }
                }
                std::cout << "commands.size = " << commands.size() << std::endl;
                co_await _state_machine->apply(commands);
                auto apply_idx = _commit_idx;
                for (auto it = _commit_waiters.begin(); it != _commit_waiters.end(); ) {
                    if (it->idx <= _commit_idx) {
                        co_await it->promise->release();
                        it = _commit_waiters.erase(it);
                    } else {
                        it ++;
                    }
                }
                for (auto it = _apply_waiters.begin(); it != _apply_waiters.end(); ) {
                    if (it->idx <= apply_idx) {
                        co_await it->promise->release();
                        it = _apply_waiters.erase(it);
                    } else {
                        it ++;
                    }
                }
                _apply_idx = std::max(_apply_idx, apply_idx);
            }
            for (auto&& m : output.messages) {
                co_await send_message(m.first, m.second);
            }
        }
    };
    raft::index_t apply_index() {
        return _apply_idx;
    }
    co_context::task<> add_entry(raft::command command) {
        auto entry = _fsm->add_entry(command);
        co_await wait_for_commit(entry.idx);
    }
    co_context::task<> read_barrier() {
        auto read_idx = _fsm->start_read_barrier(_id)->second;
        co_await wait_for_apply(read_idx);
    }
    
private:    
    struct waiter {
        raft::index_t idx;
        std::shared_ptr<co_context::channel<std::monostate>> promise;
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
    
};