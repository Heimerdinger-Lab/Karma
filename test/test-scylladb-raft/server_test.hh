#pragma once

// class 
#include "co_context/co/channel.hpp"
#include "karma-raft/raft.hh"
#include "karma-raft/server.hh"
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct cmd {
    uint32_t op;
    std::string key;
    std::string value;
    size_t size() const {
        return key.size() + value.size() + 4;
    }
    std::string to_string() const {
        return "op: " + std::to_string(op) + ", key: " + key + ", value: " + value;
    }
    static cmd from_string(const std::string& str) {
        cmd command;
        std::istringstream iss(str);
        std::string token;
        
        // Parse 'op'
        if (!(iss >> command.op)) {
            // Handle parsing error
            throw std::invalid_argument("Invalid format: 'op'");
        }
        
        // Parse 'key'
        if (!std::getline(iss, token, ',')) {
            // Handle parsing error
            throw std::invalid_argument("Invalid format: 'key'");
        }
        command.key = token.substr(token.find(":") + 1); // Extract key value
        
        // Parse 'value'
        if (!std::getline(iss, token, ',')) {
            // Handle parsing error
            throw std::invalid_argument("Invalid format: 'value'");
        }
        command.value = token.substr(token.find(":") + 1); // Extract value value
        
        return command;
    }
};
struct execute_barrier_on_leader {
    // reply_id_t reply_id;
};

struct execute_barrier_on_leader_reply {
    raft::read_barrier_reply reply;
    // reply_id_t reply_id;
};

struct add_entry_message {
    raft::command cmd;
};

struct add_entry_reply_message {
    raft::add_entry_reply reply;

};
using network_message = std::variant<raft::append_request,
      raft::append_reply,
      raft::vote_request,
      raft::vote_reply,
      raft::install_snapshot,
      raft::snapshot_reply,
      raft::timeout_now,
      raft::read_quorum,
      raft::read_quorum_reply,
      execute_barrier_on_leader,
      execute_barrier_on_leader_reply,
      add_entry_message,
      add_entry_reply_message>;

class impure_state_machine : public raft::state_machine{
public:
    impure_state_machine(raft::server_id id) 
        : _id(id) {}
    co_context::task<> apply(std::vector<raft::command_cref> command) {
        for (auto& cref : command) {
            cmd cd = cmd::from_string(cref);
            _values[cd.key] = cd.value;
        }
        co_return;
    }
    co_context::task<raft::snapshot_id> take_snapshot() {}
    void drop_snapshot(raft::snapshot_id id) {}
    co_context::task<> load_snapshot(raft::snapshot_id id) {}
    co_context::task<> abort() {}
    std::string get(std::string key) {
        return _values[key];
    }
private:    
    raft::server_id _id;
    std::map<std::string, std::string> _values;
};

class rpc : public raft::rpc {
public:
    rpc(raft::server_id id, std::function<void(raft::server_id from, raft::server_id to, network_message)> send)
        : _read_barrier_channel(std::make_shared<co_context::channel<raft::read_barrier_reply>>())
        , _send_add_entry_channel(std::make_shared<co_context::channel<raft::add_entry_reply>>())
        , _id(id)
        , _send(send){
    }
    co_context::task<> receive(raft::server_id from, network_message msg) {
        std::cout << "id = " << _id << ", from = " << from << ", msg = " << msg.index() << std::endl;
        if (std::holds_alternative<raft::append_request>(msg)) {
            _client->append_entries(from, std::get<raft::append_request>(msg));
        } else if (std::holds_alternative<raft::append_reply>(msg)) {
            _client->append_entries_reply(from, std::get<raft::append_reply>(msg));
        } else if (std::holds_alternative<raft::vote_request>(msg)) {
            _client->request_vote(from, std::get<raft::vote_request>(msg));
        } else if (std::holds_alternative<raft::vote_reply>(msg)) {
            _client->request_vote_reply(from, std::get<raft::vote_reply>(msg));
        } else if (std::holds_alternative<raft::timeout_now>(msg)) {
            _client->timeout_now_request(from, std::get<raft::timeout_now>(msg));
        } else if (std::holds_alternative<raft::read_quorum>(msg)) {
            _client->read_quorum_request(from, std::get<raft::read_quorum>(msg));
        } else if (std::holds_alternative<raft::read_quorum_reply>(msg)) {
            _client->read_quorum_reply(from, std::get<raft::read_quorum_reply>(msg));
        } else if (std::holds_alternative<execute_barrier_on_leader>(msg)) {
            auto reply = co_await _client->execute_read_barrier(from);
            _send(_id, from, msg);
        } else if (std::holds_alternative<execute_barrier_on_leader_reply>(msg)) {
            co_await _read_barrier_channel->release(std::get<execute_barrier_on_leader_reply>(msg).reply);
        } else if (std::holds_alternative<add_entry_message>(msg)) {
            auto reply = co_await _client->execute_add_entry(from, std::get<add_entry_message>(msg).cmd);
            _send(_id, from, msg);
        } else if (std::holds_alternative<add_entry_reply_message>(msg)) {
            co_await _send_add_entry_channel->release(std::get<add_entry_reply_message>(msg).reply);
        }
    }
    co_context::task<raft::snapshot_reply> send_snapshot(raft::server_id server_id, const raft::install_snapshot& snap) {
        
    }

    co_context::task<> send_append_entries(raft::server_id id, const raft::append_request& append_request) {
        _send(_id, id, append_request);
        co_return;
    }


    void send_append_entries_reply(raft::server_id id, const raft::append_reply& reply) {
        _send(_id, id, reply);
    }


    void send_vote_request(raft::server_id id, const raft::vote_request& vote_request) {
        _send(_id, id, vote_request);
    }


    void send_vote_reply(raft::server_id id, const raft::vote_reply& vote_reply) {
        _send(_id, id, vote_reply);
    }


    void send_timeout_now(raft::server_id id, const raft::timeout_now& timeout_now) {
        _send(_id, id, timeout_now);
    }

    void send_read_quorum(raft::server_id id, const raft::read_quorum& read_quorum) {
        _send(_id, id, read_quorum);
    }

    void send_read_quorum_reply(raft::server_id id, const raft::read_quorum_reply& read_quorum_reply) {
        _send(_id, id, read_quorum_reply);
    }
    // 

    co_context::task<raft::read_barrier_reply> execute_read_barrier_on_leader(raft::server_id id) {
        // 需要等
        _send(_id, id, execute_barrier_on_leader {});
        co_return co_await _read_barrier_channel->acquire();
    }

    co_context::task<raft::add_entry_reply> send_add_entry(raft::server_id id, const raft::command& cmd) {
        // 需要等
        _send(_id, id, add_entry_message{});
        co_return co_await _send_add_entry_channel->acquire();
    }
    co_context::task<raft::add_entry_reply> send_modify_config(raft::server_id id,
        const std::vector<raft::config_member>& add,
        const std::vector<raft::server_id>& del) {

    }

    void on_configuration_change(raft::server_address_set add,
            raft::server_address_set del) {

    }

    co_context::task<> abort() {

    }



private:
    raft::server_id _id;
    std::shared_ptr<co_context::channel<raft::read_barrier_reply>> _read_barrier_channel;
    std::shared_ptr<co_context::channel<raft::add_entry_reply>> _send_add_entry_channel;
    std::function<void(raft::server_id from, raft::server_id to, network_message)> _send;
};

class ps : public raft::persistence {
public:
    ps(raft::server_id id) {
        m_id = id;
        m_sd.id = 0;
        m_sd.idx = 0;
        m_sd.term = 1;
        raft::config_member member1(raft::server_address(1, "1"), true);
        raft::config_member member2(raft::server_address(2, "2"), true);
        raft::config_member member3(raft::server_address(3, "3"), true);
        m_sd.config.current.insert(member1);
        // m_sd.config.current.insert(member2);
        // m_sd.config.current.insert(member3);
    }
    ~ps() {}
    co_context::task<> store_term_and_vote(raft::term_t term, raft::server_id vote) override {
        m_term = term;
        m_vote_id = vote;
        co_return;
    }
    co_context::task<std::pair<raft::term_t, raft::server_id>> load_term_and_vote() override {
        auto p = std::pair<raft::term_t, raft::server_id>(m_term, m_vote_id);
        co_return p;
    }
    co_context::task<> store_commit_idx(raft::index_t idx) override {
        m_index = idx;
        co_return;
    }
    co_context::task<raft::index_t> load_commit_idx() override {
        co_return m_index;
    }

    co_context::task<> store_log_entries(const std::vector<raft::log_entry_ptr>& entries) override {
        m_entries = entries;
        co_return;
    }
    co_context::task<raft::log_entries> load_log() override{
        co_return m_entries;
    }  
    co_context::task<> truncate_log(raft::index_t idx) override {
        co_return;
    }
    co_context::task<> abort() override {
        co_return;
    }
    co_context::task<> store_snapshot_descriptor(const raft::snapshot_descriptor& snap, size_t preserve_log_entries) override {
        m_sd = snap;
        co_return;
    }
    co_context::task<raft::snapshot_descriptor> load_snapshot_descriptor() override {
        co_return m_sd;
    }
private:
    // just in the memory
    raft::server_id m_id;
    raft::term_t m_term;
    raft::server_id m_vote_id;
    raft::index_t m_index;
    raft::snapshot_descriptor m_sd;
    std::vector<raft::log_entry_ptr> m_entries;
};
class fd : public raft::failure_detector {
public:
    ~fd() override {}
    // Called by each server on each tick, which defaults to 10
    // per second. Should return true if the server is
    // alive. False results may impact liveness.
    bool is_alive(raft::server_id server) override{
        return true;
    }
};

class raft_server {
public:
    raft_server(raft::server_id id, impure_state_machine &sm, rpc &rpc_, std::unique_ptr<raft::server>&& svr)
        : _id(id)
        , _sm(sm)
        , _rpc(rpc_) 
        , _server(std::move(svr)){
    };
    bool is_leader() {
        return _server->is_leader();
    }
    static std::unique_ptr<raft_server> create(raft::server_id id, std::function<void(raft::server_id from, raft::server_id to, network_message)> send) {
        auto sm = std::make_unique<impure_state_machine>(id);
        auto& smm = *sm;
        auto rpc_ = std::make_unique<rpc>(id, send);
        auto& rpcc = *rpc_;
        auto ps_ = std::make_unique<ps>(id);
        auto fd_ = std::make_shared<fd>();
        raft::server::configuration config;

        auto svr = raft::create_server(id, std::move(rpc_), std::move(sm), std::move(ps_), std::move(fd_), config);
        return std::make_unique<raft_server>(id, smm, rpcc, std::move(svr));
    }
    co_context::task<> tick() {
        co_await _server->tick();
        co_return;
    }
    co_context::task<> poll() {
        // co_await _server->
    }
    co_context::task<> start() {
        co_await _server->start();
    }
    co_context::task<> put(std::string key, std::string value) {
        cmd cmd_;
        cmd_.op = 0;
        cmd_.key = key;
        cmd_.value = value;
        co_await _server->add_entry(cmd_.to_string(), raft::wait_type::applied);
    }
    co_context::task<std::string> get(std::string key) {
        co_await _server->read_barrier();
        co_return _sm.get(key);
    }
    rpc & get_rpc() {
        return _rpc;
    }
private:
    raft::server_id _id;
    impure_state_machine &_sm;
    rpc &_rpc;
    std::unique_ptr<raft::server> _server;
};

class cluster  {
public:

public: 
    cluster(uint32_t count) {
        _count = count;
        _servers.push_back(raft_server::create(100, std::bind(&cluster::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
        for (int i = 1; i <= _count; i++) {
            _servers.push_back(raft_server::create(i, std::bind(&cluster::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
        }
    }
    co_context::task<> tick() {
        // std::cout << "tick(): " << _time << std::endl;
        _time++;
        co_await tick_network();
        for (int i = 1; i <= _count; i++) {
            co_await _servers[i]->tick();
        }
        // if (_time % 1 == 0) {
        //     for (int i = 1; i <= _count; i++) {
        //         co_await _servers[i]->tick();
        //     }
        // }
        // co_await co_context::yield();
        co_return;
    }
    // co_context::task<> poll() {

    //     for (int i = 1; i <= _count; i++) {
    //         co_await _servers[i]->tick
    //     }        
    //     // co_await co_context::yield();
    // }
    co_context::task<> start() {
        for (int i = 1; i <= _count; i++) {
            co_await _servers[i]->start();
        }
        // co_context::co_spawn([](std::shared_ptr<cluster> cls) -> co_context::task<> {
        //     while(1) {
        //         std::cout << "loop" << std::endl;
        //         co_await cls->tick();
        //     }
        // }(shared_from_this()));
    }

    co_context::task<> tick_network() {
        // for (auto it = _event.begin(); it != _event.end(); ) {
        //     if (it->time <= _time) {
        //         co_await _servers[it->to]->get_rpc().receive(it->from, it->msg);
        //         it = _event.erase(it);
        //     } else {
        //         it++;
        //     }
        // }
        for (auto &item : _event) {
            co_await _servers[item.to]->get_rpc().receive(item.from, item.msg);
        }
        _event.clear();
        co_return;
    }
    static int getRandomNumber(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(gen);
    }
    void send(raft::server_id from, raft::server_id to, network_message msg) {
        // _queue.push()
        // auto delivery_time = _clock.now() + raft::logical_clock::duration{_delivery_delay(_rnd)};
        uint32_t delivery_time = _time + getRandomNumber(0, 10);
        if (to == 0) return;
        // std::cout << "from: " << from << ", to: " << to << ", time: " << delivery_time << ", "  << std::endl;
        _event.push_back(task {.time = delivery_time, .from = from, .to = to, .msg = msg});
    }
    std::shared_ptr<raft_server> get_raft_server(int idx) {
        return _servers[idx];
    }
private:
    std::vector<std::shared_ptr<raft_server>> _servers;
    uint32_t _count = 0;
    uint32_t _time = 0;

    struct task {
        // raft::logical_clock::time_point time;
        uint32_t time = 0;
        raft::server_id from;
        raft::server_id to;
        network_message msg;
    };
    
    std::vector<task> _event; 
};