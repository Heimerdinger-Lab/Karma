#pragma once
#include <flatbuffers/buffer.h>

#include <iostream>
#include <map>
#include <ratio>

#include "karma-raft/raft.hh"
#include "protocol/rpc_generated.h"
namespace service {
class raft_state_machine : public raft::state_machine {
   public:
    ~raft_state_machine() {}
    co_context::task<> apply(std::vector<raft::command> command) override {
        std::cout << "applying" << std::endl;
        for (auto& item : command) {
            std::string key, value;
            std::string sb = item.data();
            int size = item.size();
            std::cout << "2cmd_size" << size << std::endl;
            std::cout << "2cmd: " << item << std::endl;
            auto header = flatbuffers::GetRoot<karma_rpc::Command>(item.data());
            std::cout << "state machine receive: " << header->type()
                      << ", key: " << header->key()->str() << ", value: " << header->value()->str()
                      << std::endl;
            m_kv[header->key()->str()] = header->value()->str();
        }
        co_return;
    };
    co_context::task<raft::snapshot_id> take_snapshot() {
        throw std::logic_error("Function not yet implemented.");
    };
    void drop_snapshot(raft::snapshot_id id) {
        throw std::logic_error("Function not yet implemented.");
    };
    co_context::task<> load_snapshot(raft::snapshot_id id) {
        throw std::logic_error("Function not yet implemented.");
    }

    co_context::task<> abort() { throw std::logic_error("Function not yet implemented."); }
    std::string get(std::string key) { return m_kv[key]; }

   private:
    void decode_command(std::string command, std::string& key, std::string& value) {
        throw std::logic_error("Function not yet implemented.");
    }
    std::map<std::string, std::string> m_kv;
};
}  // namespace service
