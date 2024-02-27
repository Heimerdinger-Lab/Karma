#pragma once

// class 
#include "karma-raft/raft.hh"
#include "karma-raft/server.hh"
#include "karma-raft/simple_server.hh"
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

struct cmdabc {
    uint32_t op;
    std::string key;
    std::string value;
    size_t size() const {
        return key.size() + value.size() + 4;
    }
    std::string to_string() const {
        return key + " " + value;
        // return "op: " + std::to_string(op) + ", key: " + key + ", value: " + value;
    }
    static cmdabc from_string(const std::string& str) {
        cmdabc command;
        std::istringstream iss(str);
        iss >> command.key >> command.value;
        return command;
        // std::string token;
        
        // // Parse 'op'
        // if (!(iss >> command.op)) {
        //     // Handle parsing error
        //     throw std::invalid_argument("Invalid format: 'op'");
        // }
        
        // // Parse 'key'
        // if (!std::getline(iss, token, ',')) {
        //     // Handle parsing error
        //     throw std::invalid_argument("Invalid format: 'key'");
        // }
        // command.key = token.substr(token.find(":") + 1); // Extract key value
        
        // // Parse 'value'
        // if (!std::getline(iss, token, ',')) {
        //     // Handle parsing error
        //     throw std::invalid_argument("Invalid format: 'value'");
        // }
        // command.value = token.substr(token.find(":") + 1); // Extract value value
        
        // return command;
    }
};
class impure_state_machine : public raft::state_machine{
public:
    impure_state_machine(raft::server_id id) 
        : _id(id) {}
    co_context::task<> apply(std::vector<raft::command_cref> command) {
        for (auto& cref : command) {
            cmdabc cd = cmdabc::from_string(cref);
            std::cout << "apply, key: " << cd.key << ", " << cd.value << std::endl;
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
    rpc(raft::server_id id, std::function<void(raft::server_id from, raft::server_id to, raft::rpc_message)> send)
        : _id(id)
        , _send(send){
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

    co_context::task<raft::read_barrier_reply> execute_read_barrier_on_leader(raft::server_id id) {
        // 需要等
        
    }

    co_context::task<raft::add_entry_reply> send_add_entry(raft::server_id id, const raft::command& cmd) {
        // 需要等

    }
    co_context::task<raft::add_entry_reply> send_modify_config(raft::server_id id,
        const std::vector<raft::config_member>& add,
        const std::vector<raft::server_id>& del) {
            
    }

    void on_configuration_change(raft::server_address_set add,
            raft::server_address_set del) {

    }

    // 
    co_context::task<> abort() {
        co_return;
    }



private:
    raft::server_id _id;
    std::function<void(raft::server_id from, raft::server_id to, raft::rpc_message)> _send;
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
    raft_server(raft::server_id id, impure_state_machine &sm, rpc &rpc_, std::unique_ptr<sb_server>&& svr)
        : _id(id)
        , _sm(sm)
        , _rpc(rpc_) 
        , _server(std::move(svr)){
    };
    co_context::task<> receive(raft::server_id from, raft::rpc_message msg) {
        std::cout << "receive" << std::endl;
        co_await _server->receive(from, msg);
        co_return;
    }
    static std::unique_ptr<raft_server> create(raft::server_id id, std::function<void(raft::server_id from, raft::server_id to, raft::rpc_message)> send) {
        auto sm = std::make_unique<impure_state_machine>(id);
        auto& smm = *sm;
        auto rpc_ = std::make_unique<rpc>(id, send);
        auto& rpcc = *rpc_;
        auto fd_ = std::make_unique<fd>();
        auto svr = sb_server::create(id, std::move(sm), std::move(rpc_), std::move(fd_));
        return std::make_unique<raft_server>(id, smm, rpcc, std::move(svr));
    }
    co_context::task<> tick() {
        _server->tick();
        co_return;
    }
    bool is_leader() {
        return _server->is_leader();
    }
    co_context::task<> poll() {
        // co_await _server->
    }
    co_context::task<> start() {
        co_await _server->start();
        co_return;
    }
    co_context::task<> put(std::string key, std::string value) {
        cmdabc cmd_;
        cmd_.op = 0;
        cmd_.key = key;
        cmd_.value = value;
        co_await _server->add_entry(cmd_.to_string());
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
    std::unique_ptr<sb_server> _server;
};

// class cluster  {
// public:

// public: 
//     cluster(uint32_t count) {
//         _count = count;
//         _servers.push_back(raft_server::create(100, std::bind(&cluster::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
//         for (int i = 1; i <= _count; i++) {
//             _servers.push_back(raft_server::create(i, std::bind(&cluster::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
//         }
//     }
//     co_context::task<> tick() {
//         // std::cout << "tick(): " << _time << std::endl;
//         _time++;
//         co_await tick_network();
//         for (int i = 1; i <= _count; i++) {
//             co_await _servers[i]->tick();
//         }
//         // if (_time % 1 == 0) {
//         //     for (int i = 1; i <= _count; i++) {
//         //         co_await _servers[i]->tick();
//         //     }
//         // }
//         // co_await co_context::yield();
//         co_return;
//     }
//     raft::server_id who_is_leader() {
//         for (int i = 1; i <= _count; i++) {
//             if (_servers[i]->is_leader()) {
//                 return i;
//             }
//         }
//         return 0;
//     }
//     // co_context::task<> poll() {

//     //     for (int i = 1; i <= _count; i++) {
//     //         co_await _servers[i]->tick
//     //     }        
//     //     // co_await co_context::yield();
//     // }
//     co_context::task<> start() {
//         for (int i = 1; i <= _count; i++) {
//             co_await _servers[i]->start();
//         }
//         // co_context::co_spawn([](std::shared_ptr<cluster> cls) -> co_context::task<> {
//         //     while(1) {
//         //         std::cout << "loop" << std::endl;
//         //         co_await cls->tick();
//         //     }
//         // }(shared_from_this()));
//     }

//     co_context::task<> tick_network() {
//         // for (auto it = _event.begin(); it != _event.end(); ) {
//         //     if (it->time <= _time) {
//         //         co_await _servers[it->to]->get_rpc().receive(it->from, it->msg);
//         //         it = _event.erase(it);
//         //     } else {
//         //         it++;
//         //     }
//         // }
//         for (auto &item : _event) {
//             co_await _servers[item.to]->receive(item.from, item.msg);
//         }
//         _event.clear();
//         co_return;
//     }
//     static int getRandomNumber(int min, int max) {
//         static std::random_device rd;
//         static std::mt19937 gen(rd());
//         std::uniform_int_distribution<int> distribution(min, max);
//         return distribution(gen);
//     }
//     void send(raft::server_id from, raft::server_id to, raft::rpc_message msg) {
//         // _queue.push()
//         // auto delivery_time = _clock.now() + raft::logical_clock::duration{_delivery_delay(_rnd)};
//         uint32_t delivery_time = 0;
//         if (to == 0) return;
//         // std::cout << "from: " << from << ", to: " << to << ", time: " << delivery_time << ", "  << std::endl;
//         _event.push_back(task {.time = delivery_time, .from = from, .to = to, .msg = msg});
//     }
//     std::shared_ptr<raft_server> get_raft_server(int idx) {
//         return _servers[idx];
//     }
// private:
//     std::vector<std::shared_ptr<raft_server>> _servers;
//     uint32_t _count = 0;
//     uint32_t _time = 0;

//     struct task {
//         // raft::logical_clock::time_point time;
//         uint32_t time = 0;
//         raft::server_id from;
//         raft::server_id to;
//         raft::rpc_message msg;
//     };
    
//     std::vector<task> _event; 
// };