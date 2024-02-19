#pragma once

// class 
#include "scylladb-raft/raft.hh"
#include <functional>
#include <memory>
#include <random>
#include <vector>

class network {
public:
    void send() {

    }
    void tick() {

    }
private:

};


class impure_state_machine {

};
class rpc : public raft::rpc {

};
class raft_server {
    static std::shared_ptr<raft_server> create() {

    };
    void tick() {
        
    }
private:
    raft::server_id _id;
    impure_state_machine &_sm;
    rpc &_rpc;
};



class cluster {
public: 
    cluster(uint32_t count) {
        _count = count;
        for (int i = 0; i < _count; i++) {
            _servers.push_back(raft_server::create());
        }
    }
    void tick() {
        for (int i = 0; i < _count; i++) {
            _servers[i].tick();
        }
    }
    void start() {
        for (int i = 0; i < _count; i++) {
            _servers[i].start();
        }
        co_context::co_spawn([this]() -> co_context::task<> {
            while(1) {
                tick();
                co_await co_context::yield();
            }
        }());
    }


    void send() {

    }
    std::shared_ptr<raft_server> get_raft_server(int idx) {
        return _servers[idx];
    }
private:
    std::vector<std::shared_ptr<raft_server>> _servers;
    uint32_t _count;
    network _network;
};