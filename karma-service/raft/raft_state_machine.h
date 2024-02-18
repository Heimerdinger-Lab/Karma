#pragma once
// #include "karma-raft/server.h"
#include "scylladb-raft/server.hh"
#include <map>
class raft_state_machine : public raft::state_machine {
public:
    ~raft_state_machine() {}
    co_context::task<> apply(std::vector<raft::command_cref> command) {
        for (auto& item : command) {
            std::string key, value;
            decode_command(item, key, value);
            m_kv[key] = value;
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

    co_context::task<> abort() {
        throw std::logic_error("Function not yet implemented.");
    }
    std::string get(std::string key) {
        return m_kv[key];
    }
private:
    void decode_command(std::string command, std::string& key, std::string& value) {
        throw std::logic_error("Function not yet implemented.");
    }
    std::map<std::string, std::string> m_kv;
};