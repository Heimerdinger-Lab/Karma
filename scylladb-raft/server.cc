#include "co_context/co/channel.hpp"
#include "scylladb-raft/fsm.hh"
#include "server.hh"
#include "raft.hh"
#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <variant>
namespace raft { 
class server_impl : public rpc_server, public server { 
public:
    explicit server_impl(server_id uuid, std::unique_ptr<rpc> rpc,
        std::unique_ptr<state_machine> state_machine, std::unique_ptr<persistence> persistence,
        std::shared_ptr<failure_detector> failure_detector, server::configuration config);

    server_impl(server_impl&&) = delete;

    ~server_impl() {}
    // implement rpc_server
    // 对内
    void append_entries(server_id from, append_request append_request) override;
    void append_entries_reply(server_id from, append_reply reply) override;
    void request_vote(server_id from, vote_request vote_request) override;
    void request_vote_reply(server_id from, vote_reply vote_reply) override;
    void timeout_now_request(server_id from, timeout_now timeout_now) override;
    void read_quorum_request(server_id from, struct read_quorum read_quorum) override;
    void read_quorum_reply(server_id from, struct read_quorum_reply read_quorum_reply) override;
    co_context::task<read_barrier_reply> execute_read_barrier(server_id) override;
    co_context::task<add_entry_reply> execute_add_entry(server_id from, command cmd) override;
    // co_context::task<snapshot_reply> apply_snapshot(server_id from, install_snapshot snp) override;
    // co_context::task<read_barrier_reply> execute_read_barrier(server_id from);
    // implement server
    // 对外
    // add_entry就是给raft group添加command
    // read_barrier就是等待状态机更新完
    // 直接从状态机中读值即可。
    co_context::task<> add_entry(command command, wait_type type) override;
    co_context::task<> start() override;
    co_context::task<> abort(std::string reason) override;
    co_context::task<> wait_for_entry(entry_id eid, wait_type type);
    co_context::task<> wait_for_apply(index_t idx); 
    co_context::task<read_barrier_reply> get_read_idx(server_id leader);
    bool is_alive() const override;
    term_t get_current_term() const override;
    co_context::task<> read_barrier() override;
    void wait_until_candidate() override;
    co_context::task<> wait_election_done() override;
    co_context::task<> wait_log_idx_term(std::pair<index_t, term_t> idx_log) override;
    std::pair<index_t, term_t> log_last_idx_term() override;
    void elapse_election() override;
    bool is_leader() override;
    raft::server_id current_leader() const override;
    void tick() override;
    raft::server_id id() const override;
    void set_applier_queue_max_size(size_t queue_max_size) override;
    co_context::task<entry_id> add_entry_on_leader(command command);
    size_t max_command_size() const override;
    co_context::task<> signal_applied();
private:
    std::unique_ptr<rpc> _rpc;
    std::unique_ptr<state_machine> _state_machine;
    std::unique_ptr<persistence> _persistence;
    std::shared_ptr<failure_detector> _failure_detector;
    std::unique_ptr<fsm> _fsm;
    server_id _id;
    server::configuration _config;
    index_t _applied_idx;
    struct op_status {
        term_t term; // term the entry was added with
        std::unique_ptr<co_context::channel<std::monostate, 1>> chan;
        // op_status() = default;
        // promise<> done; // notify when done here 
    };
    std::map<index_t, op_status> _awaited_commits;
    std::map<index_t, op_status> _awaited_applies;
    struct awaited_index {
        std::unique_ptr<co_context::channel<std::monostate, 1>> chan;
    };
    std::multimap<index_t, awaited_index> _awaited_indexes;
    using applier_fiber_message = std::variant<
        std::vector<log_entry_ptr>,
        snapshot_descriptor>;
    std::queue<applier_fiber_message> _apply_entries;
private:
    co_context::task<> io_fiber(index_t stable_idx);
    co_context::task<> applier_fiber();
    template <typename Message> 
    void send_message(server_id id, Message m);
private:


};

inline void server_impl::append_entries(server_id from, append_request append_request) {
    _fsm->step(from, std::move(append_request));
}

void server_impl::append_entries_reply(server_id from, append_reply reply) {
    _fsm->step(from, std::move(reply));
}

void server_impl::request_vote(server_id from, vote_request vote_request) {
    _fsm->step(from, std::move(vote_request));
}

void server_impl::request_vote_reply(server_id from, vote_reply vote_reply) {
    _fsm->step(from, std::move(vote_reply));
}

void server_impl::timeout_now_request(server_id from, timeout_now timeout_now) {
    _fsm->step(from, std::move(timeout_now));
}

void server_impl::read_quorum_request(server_id from, struct read_quorum read_quorum) {
    _fsm->step(from, std::move(read_quorum));
}

void server_impl::read_quorum_reply(server_id from, struct read_quorum_reply read_quorum_reply) {
    _fsm->step(from, std::move(read_quorum_reply));
}

inline co_context::task<read_barrier_reply> server_impl::execute_read_barrier(server_id from) {
    // check_not_aborted();

    // logger.trace("[{}] execute_read_barrier start", _id);

    std::optional<std::pair<read_id, index_t>> rid;
    rid = _fsm->start_read_barrier(from);
    co_return rid->second;
}

inline co_context::task<add_entry_reply> server_impl::execute_add_entry(server_id from, command cmd) {
    if (from != _id && !_fsm->get_configuration().contains(from)) {
        // Do not accept entries from servers removed from the
        // configuration.
        co_return add_entry_reply{not_a_member{fmt::format("Add entry from {} was discarded since "
                                                         "it is not part of the configuration", from)}};
    }
    // logger.trace("[{}] adding a forwarded entry from {}", id(), from);
    try {
        co_return add_entry_reply{co_await add_entry_on_leader(std::move(cmd))};
    } catch (raft::not_a_leader& e) {
        co_return add_entry_reply{transient_error{std::current_exception(), e.leader}};
    }
}

inline co_context::task<> server_impl::add_entry(command command, wait_type type) {
    if (command.size() > _config.max_command_size) {
        // logger.trace("[{}] add_entry command size exceeds the limit: {} > {}",
        //              id(), command.size(), _config.max_command_size);
        throw command_is_too_big_error(command.size(), _config.max_command_size);
    }
    // _stats.add_command++;

    // logger.trace("[{}] an entry is submitted", id());
    if (!_config.enable_forwarding) {
        if (const auto leader = _fsm->current_leader(); leader != _id) {
            throw not_a_leader{leader};
        }
        auto eid = co_await add_entry_on_leader(std::move(command));
        co_return co_await wait_for_entry(eid, type);
    }
}

inline co_context::task<> server_impl::wait_for_entry(entry_id eid, wait_type type) {
    // The entry may have been already committed and even applied
    // in case it was forwarded to the leader. In this case
    // waiting for it is futile.
    if (eid.idx <= _fsm->commit_idx()) {
        if ((type == wait_type::committed) ||
            (type == wait_type::applied && eid.idx <= _applied_idx)) {

            auto term = _fsm->log_term_for(eid.idx);

            // _stats.waiters_awoken++;

            if (!term) {
                // The entry at index `eid.idx` got truncated away.
                // Still, if the last snapshot's term is the same as `eid.term`, we can deduce
                // that our entry `eid` got committed at index `eid.idx` and not some different entry.
                // Indeed, let `snp_idx` be the last snapshot index (`snp_idx >= eid.idx`). Consider
                // the entry that was committed at `snp_idx`; it had the same term as the snapshot's term,
                // `snp_term`. If `eid.term == snp_term`, then we know that the entry at `snp_idx` was
                // created by the same leader as the entry `eid`. A leader doesn't replace an entry
                // that it previously appended, so when it appended the `snp_idx` entry, the entry at
                // `eid.idx` was still `eid`. By the Log Matching Property, every log that had the entry
                // `(snp_idx, snp_term)` also had the entry `eid`. Thus when the snapshot at `snp_idx`
                // was created, it included the entry `eid`.
                auto snap_idx = _fsm->log_last_snapshot_idx();
                auto snap_term = _fsm->log_term_for(snap_idx);
                assert(snap_term);
                assert(snap_idx >= eid.idx);
                if (type == wait_type::committed && snap_term == eid.term) {
                    // logger.trace("[{}] wait_for_entry {}.{}: entry got truncated away, but has the snapshot's term"
                    //              " (snapshot index: {})", id(), eid.term, eid.idx, snap_idx);
                    co_return;

                    // We don't do this for `wait_type::applied` - see below why.
                }

                // logger.trace("[{}] wait_for_entry {}.{}: entry got truncated away", id(), eid.term, eid.idx);
                throw commit_status_unknown();
            }

            if (*term != eid.term) {
                throw dropped_entry();
            }

            if (type == wait_type::applied && _fsm->log_last_snapshot_idx() >= eid.idx) {
                // We know the entry was committed but the wait type is `applied`
                // and we don't know if the entry was applied with `state_machine::apply`
                // (we may've loaded a snapshot before we managed to apply the entry).
                // As specified by `add_entry`, throw `commit_status_unknown` in this case.
                //
                // FIXME: replace this with a different exception type - `commit_status_unknown`
                // gives too much uncertainty while we know that the entry was committed
                // and had to be applied on at least one server. Some callers of `add_entry`
                // need to know only that the current state includes that entry, whether it was done
                // through `apply` on this server or through receiving a snapshot.
                throw commit_status_unknown();
            }

            co_return;
        }
    }

    std::map<index_t, op_status>& container = type == wait_type::committed ? _awaited_commits : _awaited_applies;
    // container[eid.idx] = op_status
    // logger.trace("[{}] waiting for entry {}.{}", id(), eid.term, eid.idx);

    // This will track the commit/apply status of the entry
    // std::pair<index_t, op_status> item = std::make_pair(std::move(eid.idx), op_status{.term = eid.term, .chan = co_context::channel<std::monostate, 1>()});
    auto [it, inserted] = container.emplace(eid.idx, op_status {.term = eid.term});
    
    // auto it = container.insert(std::pair<index_t, op_status>((index_t)eid.idx, eid.term));
    // container[eid.idx] = op_status{eid.term};
    // assert(inserted);
    co_await it->second.chan->acquire();
    co_return;
}

inline co_context::task<> server_impl::wait_for_apply(index_t idx) {
    if (idx > _applied_idx) {
        // The index is not applied yet. Wait for it.
        // This will be signalled when read_idx is applied
        // awaited_index x;
        auto it = _awaited_indexes.emplace(std::move(idx), awaited_index{{}});
        // _awaited_indexes[idx] = x;
        // auto it = _awaited_indexes.insert(std::make_pair<uint64_t, struct awaited_index>(std::move(idx), std::move(awaited_index{{}})));
        co_await it->second.chan->acquire();
    }
}

inline co_context::task<read_barrier_reply> server_impl::get_read_idx(server_id leader) {
    // return execute_read_barrier(_id);
    if (_id == leader) {
        return execute_read_barrier(_id);
    } else {
        return _rpc->execute_read_barrier_on_leader(leader);
    }
}

inline co_context::task<> server_impl::read_barrier() {
    // logger.trace("[{}] read_barrier start", _id);
    index_t read_idx;

    auto applied = _applied_idx;
    read_barrier_reply res;
    
    res = co_await get_read_idx(_fsm->current_leader());
    
    read_idx = std::get<index_t>(res);
    // logger.trace("[{}] read_barrier read index {}, applied index {}", _id, read_idx, _applied_idx);
    co_return co_await wait_for_apply(read_idx);
}


inline co_context::task<entry_id> server_impl::add_entry_on_leader(command cmd) {
    try {
        const log_entry& e = _fsm->add_entry(std::move(cmd));
        co_return entry_id{.term = e.term, .idx = e.idx};
    } catch (const not_a_leader&) {
        // the semaphore is already destroyed, prevent memory_permit from accessing it
        throw;
    }
}

inline co_context::task<> server_impl::io_fiber(index_t last_stable) {
    while (true) {
        auto batch = co_await _fsm->poll_output();
        if (batch.term_and_vote) {
            // fsm相较于上次poll的时候，term或者vote发生了变化
            co_await _persistence->store_term_and_vote(batch.term_and_vote->first, batch.term_and_vote->second);
                
        }
        if (batch.log_entries.size()) {
            auto& entries = batch.log_entries;

            if (last_stable >= entries[0]->idx) {
                co_await _persistence->truncate_log(entries[0]->idx);
                // _stats.truncate_persisted_log++;
            }

            // utils::get_local_injector().inject("store_log_entries/test-failure",
            //     [] { throw std::runtime_error("store_log_entries/test-failure"); });

            // Combine saving and truncating into one call?
            // will require persistence to keep track of last idx
            co_await _persistence->store_log_entries(entries);

            last_stable = (*entries.crbegin())->idx;
            // _stats.persisted_log_entries += entries.size();
        }
            // After entries are persisted we can send messages.
        for (auto&& m : batch.messages) {
            try {
                send_message(m.first, std::move(m.second));
            } catch(...) {
                // Not being able to send a message is not a critical error
                // logger.debug("[{}] io_fiber failed to send a message to {}: {}", _id, m.first, std::current_exception());
            }
        }
        // Process committed entries.
        if (batch.committed.size()) {
            co_await _persistence->store_commit_idx(batch.committed.back()->idx);
            // _stats.queue_entries_for_apply += batch.committed.size();
            _apply_entries.push(std::move(batch.committed));
        }
        // if (batch.max_read_id_with_quorum) {
        //     while (!_reads.empty() && _reads.front().id <= batch.max_read_id_with_quorum) {
        //         _reads.front().promise.set_value(_reads.front().idx);
        //         _reads.pop_front();
        //     }
        // }

    };
}
co_context::task<> server_impl::signal_applied() {
    auto it = _awaited_indexes.begin();

    while (it != _awaited_indexes.end()) {
        if (it->first > _applied_idx) {
            break;
        }
        // it->second.promise.set_value();
        co_await it->second.chan->release(std::monostate{});
        it = _awaited_indexes.erase(it);
    }
}
inline co_context::task<> server_impl::applier_fiber() {
    // while (true) {
    //     auto v =  _apply_entries.front();
    //     _apply_entries.pop();
    //     std::visit([this](std::vector<log_entry_ptr>& batch) -> co_context::task<> {
    //         if (batch.empty()) {
    //             co_return;
    //         };
    //         index_t last_idx = batch.back()->idx;
    //         term_t last_term = batch.back()->term;
    //         // batch.back()->data;
    //         std::vector<command_cref> commands;
    //         // commands.reserve(batch.size());
    //         for (auto& i : batch) {
    //             commands.push_back(std::get<command_t>(i->data));
    //         }
    //         m_state_machine->apply(commands);
    //         m_applied_idx = last_idx;
    //         m_applied_index_changed.notify_all();
    //     }, v);
    // }
        while (true) {
            auto v = _apply_entries.front();
            _apply_entries.pop();
            if (std::holds_alternative<std::vector<log_entry_ptr>>(v)) {
                auto& batch = std::get<std::vector<log_entry_ptr>>(v);
                if (batch.empty()) {
                    // logger.trace("[{}] applier fiber: received empty batch", _id);
                    co_return;
                }

                // Completion notification code assumes that previous snapshot is applied
                // before new entries are committed, otherwise it asserts that some
                // notifications were missing. To prevent a committed entry to
                // be notified before an earlier snapshot is applied do both
                // notification and snapshot application in the same fiber
                // notify_waiters(_awaited_commits, batch);

                std::vector<command_cref> commands;
                commands.reserve(batch.size());

                index_t last_idx = batch.back()->idx;
                term_t last_term = batch.back()->term;
                assert(last_idx == _applied_idx + batch.size());

                // boost::range::copy(
                //        batch |
                //        boost::adaptors::filtered([] (log_entry_ptr& entry) { return std::holds_alternative<command>(entry->data); }) |
                //        boost::adaptors::transformed([] (log_entry_ptr& entry) { return std::cref(std::get<command>(entry->data)); }),
                //        std::back_inserter(commands));

                auto size = commands.size();
                if (size) {
                    co_await _state_machine->apply(std::move(commands));
                }

               _applied_idx = last_idx;
            //    _applied_index_changed.broadcast();
            //    notify_waiters(_awaited_applies, batch);

               // It may happen that _fsm has already applied a later snapshot (from remote) that we didn't yet 'observe'
               // (i.e. didn't yet receive from _apply_entries queue) but will soon. We avoid unnecessary work
               // of taking snapshots ourselves but comparing our last index directly with what's currently in _fsm.
               auto last_snap_idx = _fsm->log_last_snapshot_idx();

               // Error injection to be set with one_shot
            //    utils::get_local_injector().inject("raft_server_snapshot_reduce_threshold",
            //        [this] { _config.snapshot_threshold = 3; _config.snapshot_trailing = 1; });

            //    bool force_snapshot = utils::get_local_injector().enter("raft_server_force_snapshot");

            };

            co_await signal_applied();
        }
};

template <typename Message>
void server_impl::send_message(server_id id, Message m) {
    std::visit([this, id] (auto&& m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T, append_reply>) {
            _rpc->send_append_entries_reply(id, m);
        } else if constexpr (std::is_same_v<T, append_request>) {
            //  co_await _rpc->send_append_entries(id, m);
        } else if constexpr (std::is_same_v<T, vote_request>) {
            _rpc->send_vote_request(id, m);
        } else if constexpr (std::is_same_v<T, vote_reply>) {
            _rpc->send_vote_reply(id, m);
        } else if constexpr (std::is_same_v<T, timeout_now>) {
            _rpc->send_timeout_now(id, m);
        } else if constexpr (std::is_same_v<T, struct read_quorum>) {
            _rpc->send_read_quorum(id, std::move(m));
        } else if constexpr (std::is_same_v<T, struct read_quorum_reply>) {
            _rpc->send_read_quorum_reply(id, std::move(m));
        } else {
            // static_assert(!sizeof(T*), "not all message types are handled");
        }
    }, std::move(m));
}
}
