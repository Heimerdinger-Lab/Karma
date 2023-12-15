#include "karma-raft/server_impl.h"
#include "karma-raft/raft.h"
#include "karma-raft/fsm.h"
#include <queue>

namespace raft {
void server_impl::append_entries(server_id from, append_request append_request) {
    m_fsm->step(from, std::move(append_request));
}

void server_impl::append_entries_reply(server_id from, append_reply reply) {
    m_fsm->step(from, std::move(reply));
}

void server_impl::request_vote(server_id from, vote_request vote_request) {
    m_fsm->step(from, std::move(vote_request));
}

void server_impl::request_vote_reply(server_id from, vote_reply vote_reply) {
    m_fsm->step(from, std::move(vote_reply));
}

void server_impl::timeout_now_request(server_id from, term_t current_term) {
    m_fsm->step(from, std::move(current_term));
}

template <typename Message>
void server_impl::send_message(server_id id, Message m) {
    // 在io_fiber中，会把message通过rpc发送出去，就是调用这个函数去发送
    // 这里就会调用m_rpc的sendxxx方法去发送
    std::visit([this, id] (auto&& m) {

    });
}


co_context::task<> server_impl::start() {
    // auto [term, vote] = co_await m_persistence->load_term_and_vote();
    // auto log_entries = co_await m_persistence->load_log();
    // auto log_ = log(log_entries);
    // auto commit_idx = co_await m_persistence->load_commit_idx();
    // index_t stable_idx = log_.stable_idx();
    // m_fsm = std::make_unique<fsm>(m_server_id, term, vote, std::move(log_), commit_idx);
    // m_applied_idx = index_t(0);
    // co_spawn(io_fiber(stable_idx));
    // co_spawn(applier_fiber());
    // co_await m_applied_index_changed_mtx.lock();
    // while (m_applied_idx < commit_idx) {
    //     // 这就是re apply 那些之前没有做完的entries
    //     m_applied_index_changed.wait(m_applied_index_changed_mtx);
    // }
    // m_applied_index_changed_mtx.unlock();
    co_return;
}
void server_impl::tick() {
    m_fsm->tick();
}
server_id server_impl::id() const {
    return m_server_id;
}
// 主要作用是将raftlog中的entries持久化
co_context::task<> server_impl::io_fiber(index_t stable_idx) {
    while (true) {
        auto batch = co_await m_fsm->poll_output();
        if (batch.term_and_vote) {
            // fsm相较于上次poll的时候，term或者vote发生了变化
            m_persistence->store_term_and_vote(batch.term_and_vote->first, batch.term_and_vote->second);
        }
        if (!batch.log_entries.empty()) {
            // 开始处理entries了，主要是将它们持久化

        }
        for (auto&& m : batch.messages) {
            // 开始处理消息的分发了
        }
        if (!batch.committed_entries.empty()) {
            // 开始处理已经提交的entries，主要是去apply它
        }
    };
}

// 主要作用是将已经持久化的entries去apply到状态机上
co_context::task<> server_impl::applier_fiber() {
    
    while (true) {
        auto v = co_await m_apply_entries.acquire();
        std::visit([this](log_entry_vec& batch) -> co_context::task<> {
            if (batch.empty()) {
                co_return;
            };
            index_t last_idx = batch.back()->idx;
            term_t last_term = batch.back()->term;
            // batch.back()->data;
            std::vector<command_cref> commands;
            // commands.reserve(batch.size());
            for (auto& i : batch) {
                commands.push_back(std::get<command_t>(i->data));
            }
            m_state_machine->apply(commands);
            m_applied_idx = last_idx;
            m_applied_index_changed.notify_all();
        }, v);
    }
}
}