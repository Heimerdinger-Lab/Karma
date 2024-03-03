#include "raft_server.h"

#include "co_context/io_context.hpp"
service::raft_server::raft_server(raft::server_id id, std::unique_ptr<raft::state_machine> sm,
                                  std::unique_ptr<raft::rpc> rpc_,
                                  std::unique_ptr<raft::failure_detector> fd_,
                                  std::vector<raft::config_member> members)
    : _id(id),
      _state_machine(std::move(sm)),
      _rpc(std::move(rpc_)),
      _failure_detector(std::move(fd_)),
      _members(members) {}

co_context::task<> service::raft_server::start() {
    raft::snapshot_descriptor snp;
    snp.id = 0;
    snp.idx = 0;
    snp.term = 0;
    BOOST_LOG_TRIVIAL(trace) << "Raft server starting";

    auto log = raft::log(snp, std::move(_entries));
    _fsm = std::make_unique<raft::fsm>(
        _id, _term, _vote_id, log, _commit_idx, *_failure_detector,
        raft::fsm_config{
            .append_request_threshold = 1024, .max_log_size = 1024, .enable_prevoting = false});
    co_context::co_spawn(io_fiber());
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

co_context::task<> service::raft_server::io_fiber() {
    BOOST_LOG_TRIVIAL(trace) << "io_fiber start" << std::endl;
    while (true) {
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
            std::vector<raft::command> commands;
            for (auto& item : output.committed) {
                if (std::holds_alternative<raft::command>(item->data)) {
                    auto cd = std::get<raft::command>(item->data);
                    commands.push_back(cd);
                }
            }
            BOOST_LOG_TRIVIAL(trace) << "Raft server will apply " << commands.size()
                                     << " command(s) to state machine" << std::endl;
            co_await _state_machine->apply(commands);
            auto apply_idx = _commit_idx;
            for (auto it = _commit_waiters.begin(); it != _commit_waiters.end();) {
                if (it->idx <= _commit_idx) {
                    co_await it->promise->release();
                    it = _commit_waiters.erase(it);
                } else {
                    it++;
                }
            }
            for (auto it = _apply_waiters.begin(); it != _apply_waiters.end();) {
                if (it->idx <= apply_idx) {
                    co_await it->promise->release();
                    it = _apply_waiters.erase(it);
                } else {
                    it++;
                }
            }
            _apply_idx = std::max(_apply_idx, apply_idx);
        }
        for (auto&& m : output.messages) {
            co_await send_message(m.first, m.second);
        }
    }
};

co_context::task<> service::raft_server::cli_write(std::string key, std::string value) {
    flatbuffers::FlatBufferBuilder cmd_builder;
    auto key_ = cmd_builder.CreateString(key);
    auto value_ = cmd_builder.CreateString(value);
    auto cmd = karma_rpc::CreateCommand(cmd_builder, karma_rpc::CommandType_VALUE, key_, value_);
    cmd_builder.Finish(cmd);
    auto buffer = cmd_builder.GetBufferPointer();
    int size = cmd_builder.GetSize();
    std::string cmd_str;
    cmd_str.append(buffer, buffer + size);
    co_await add_entry(cmd_str);
}

co_context::task<std::string> service::raft_server::cli_read(std::string key) {
    co_await read_barrier();
    co_return "value02";
};
