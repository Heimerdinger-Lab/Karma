#include "raft_server.h"

#include <optional>

#include "co_context/io_context.hpp"
#include "karma-raft/raft.hh"
service::raft_server::raft_server(raft::server_id id,
                                  std::unique_ptr<service::raft_state_machine> sm,
                                  std::unique_ptr<service::raft_rpc> rpc_,
                                  std::unique_ptr<raft::failure_detector> fd_,
                                  std::vector<raft::config_member> members)
    : _id(id),
      _state_machine(std::move(sm)),
      _rpc(std::move(rpc_)),
      _failure_detector(std::move(fd_)),
      _members(members) {}

co_context::task<> service::raft_server::start() {
    // TODO: Use real snapshot
    raft::snapshot_descriptor snp;
    snp.id = 0;
    snp.idx = 0;
    snp.term = 0;
    //
    auto [term, vote] = co_await _persistence->load_term_and_vote();
    auto log_entries = co_await _persistence->load_log();
    raft::index_t commit_idx = 0;

    BOOST_LOG_TRIVIAL(trace) << "Raft server starting";

    auto log = raft::log(snp, std::move(log_entries));
    _fsm = std::make_unique<raft::fsm>(
        _id, static_cast<raft::term_t>(term), static_cast<raft::server_id>(vote), log, commit_idx,
        *_failure_detector,
        raft::fsm_config{
            .append_request_threshold = 1024, .max_log_size = 1024, .enable_prevoting = false});
    _apply_idx = 0;

    co_context::co_spawn(io_fiber(0));
    co_context::co_spawn(apply_fiber());
    co_return;
}

void service::raft_server::receive(raft::server_id from, raft::rpc_message msg) {
    if (std::holds_alternative<raft::append_request>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an append_request" << std::endl;
        _fsm->step(from, std::move(std::get<raft::append_request>(msg)));
    } else if (std::holds_alternative<raft::append_reply>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an append_reply" << std::endl;
        _fsm->step(from, std::move(std::get<raft::append_reply>(msg)));
    } else if (std::holds_alternative<raft::vote_request>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an vote_request" << std::endl;
        _fsm->step(from, std::move(std::get<raft::vote_request>(msg)));
    } else if (std::holds_alternative<raft::vote_reply>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an vote_reply" << std::endl;
        _fsm->step(from, std::move(std::get<raft::vote_reply>(msg)));
    } else if (std::holds_alternative<raft::timeout_now>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an timeout_now" << std::endl;
        _fsm->step(from, std::move(std::get<raft::timeout_now>(msg)));
    } else if (std::holds_alternative<raft::read_quorum>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an read_quorum" << std::endl;
        _fsm->step(from, std::move(std::get<raft::read_quorum>(msg)));
    } else if (std::holds_alternative<raft::read_quorum_reply>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server receive an read_quorum_reply" << std::endl;
        _fsm->step(from, std::move(std::get<raft::read_quorum_reply>(msg)));
    };
}

co_context::task<> service::raft_server::wait_for_commit(raft::index_t idx) {
    auto channel = std::make_unique<co_context::channel<std::monostate>>();
    waiter w{.idx = idx, .promise = channel.get()};
    _commit_waiters.push_back(w);
    co_await w.promise->acquire();
}

co_context::task<> service::raft_server::wait_for_apply(raft::index_t idx) {
    if (idx >= apply_index()) {
        co_return;
    }
    auto channel = std::make_unique<co_context::channel<std::monostate>>();
    waiter w{.idx = idx, .promise = channel.get()};
    _apply_waiters.push_back(w);
    co_await w.promise->acquire();
}

template <typename Message>
co_context::task<> service::raft_server::send_message(raft::server_id id, Message msg) {
    if (std::holds_alternative<raft::append_request>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an append_request" << std::endl;
        co_await _rpc->send_append_entries(id, std::get<raft::append_request>(msg));
    } else if (std::holds_alternative<raft::append_reply>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an append_reply" << std::endl;
        co_await _rpc->send_append_entries_reply(id, std::get<raft::append_reply>(msg));
    } else if (std::holds_alternative<raft::vote_request>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an vote_request" << std::endl;
        co_await _rpc->send_vote_request(id, std::get<raft::vote_request>(msg));
    } else if (std::holds_alternative<raft::vote_reply>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an vote_reply" << std::endl;
        co_await _rpc->send_vote_reply(id, std::get<raft::vote_reply>(msg));
    } else if (std::holds_alternative<raft::timeout_now>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an timeout_now" << std::endl;
        co_await _rpc->send_timeout_now(id, std::get<raft::timeout_now>(msg));
    } else if (std::holds_alternative<raft::read_quorum>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an read_quorum" << std::endl;
        co_await _rpc->send_read_quorum(id, std::get<raft::read_quorum>(msg));
    } else if (std::holds_alternative<raft::read_quorum_reply>(msg)) {
        BOOST_LOG_TRIVIAL(trace) << "Raft server send an read_quorum_reply" << std::endl;
        co_await _rpc->send_read_quorum_reply(id, std::get<raft::read_quorum_reply>(msg));
    }
    co_return;
}

co_context::task<> service::raft_server::io_fiber(raft::index_t last_stable) {
    BOOST_LOG_TRIVIAL(trace) << "io_fiber start" << std::endl;
    while (true) {
        auto output = co_await _fsm->poll_output();
        if (output.term_and_vote) {
            co_await _persistence->store_term_and_vote(output.term_and_vote->first,
                                                       output.term_and_vote->second);
        }
        if (output.log_entries.size()) {
            auto entries = output.log_entries;
            if (last_stable >= entries[0]->idx) {
                co_await _persistence->truncate_log(entries[0]->idx);
            }
            co_await _persistence->store_log_entries(entries);
            last_stable = (*entries.crbegin())->idx;
        }
        if (output.committed.size()) {
            // store the commit index and sent they to apply fiber.
            co_await _persistence->store_commit_idx(output.committed.back()->idx);
            co_await _apply_entries.release(std::move(output.committed));
        }
        for (auto&& m : output.messages) {
            co_await send_message(m.first, m.second);
        }
        if (output.max_read_id_with_quorum) {
            // wake up the read_idx awaiter
            for (auto it = _active_reads.begin(); it != _active_reads.end();) {
                if (it->id <= output.max_read_id_with_quorum) {
                    co_await it->promise->release();
                    it = _active_reads.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
};

co_context::task<> service::raft_server::apply_fiber() {
    while (true) {
        auto batch = co_await _apply_entries.acquire();
        // to wake up the commit awaiter
        auto commit_idx = batch.back()->idx;
        for (auto it = _commit_waiters.begin(); it != _commit_waiters.end();) {
            if (it->idx <= commit_idx) {
                co_await it->promise->release();
                it = _commit_waiters.erase(it);
            } else {
                it++;
            }
        }
        // apply the committed entries to state machine
        auto last_idx = batch.back()->idx;
        auto last_term = batch.back()->term;
        assert(last_idx == _apply_idx + batch.size());
        std::vector<raft::command> commands;
        for (auto& item : batch) {
            if (std::holds_alternative<raft::command>(item->data)) {
                auto cd = std::get<raft::command>(item->data);
                commands.push_back(cd);
            }
        }
        co_await _state_machine->apply(commands);
        _apply_idx = last_idx;
        // to wake up the apply awaiter
        for (auto it = _apply_waiters.begin(); it != _apply_waiters.end();) {
            if (it->idx <= _apply_idx) {
                co_await it->promise->release();
                it = _apply_waiters.erase(it);
            } else {
                it++;
            }
        }
    }
}

co_context::task<bool> service::raft_server::cli_write(std::string key, std::string value) {
    //
    auto leader_id = _fsm->current_leader();
    if (leader_id == raft::server_id{}) {
        co_return false;
    }
    if (leader_id == _id) {
        // 我就是女王，自信放光芒
        flatbuffers::FlatBufferBuilder cmd_builder;
        auto key_ = cmd_builder.CreateString(key);
        auto value_ = cmd_builder.CreateString(value);
        auto cmd =
            karma_rpc::CreateCommand(cmd_builder, karma_rpc::CommandType_VALUE, key_, value_);
        cmd_builder.Finish(cmd);
        auto buffer = cmd_builder.GetBufferPointer();
        int size = cmd_builder.GetSize();
        std::string cmd_str;
        cmd_str.append(buffer, buffer + size);
        auto& entry = _fsm->add_entry(cmd_str);
        co_await wait_for_commit(entry.idx);
        co_return true;
    } else {
        // 呜呜呜我不是leader
        auto reply = co_await _rpc->forward_add_entry(leader_id, key, value);
        if (reply.index() == 0) {
            auto commit_index = std::get<raft::entry_id>(reply);
            co_await wait_for_commit(commit_index.idx);
            co_return true;
        } else {
            co_return false;
        }
    }
}

co_context::task<std::optional<std::string>> service::raft_server::cli_read(std::string key) {
    auto ret = co_await read_barrier();
    if (ret) {
        co_return std::nullopt;
    }
    auto value = _state_machine->get(key);
    co_return value;
};
