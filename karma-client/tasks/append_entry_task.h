#include "protocol/rpc_generated.h"
#include "task.h"
#include <cstdint>
#include <variant>
#include <vector>

namespace client {
struct log_entry {
    uint64_t term;
    uint64_t index;
    std::string command;
};
class append_entry_request : public task {
public:
    std::shared_ptr<transport::frame> gen_frame() override {
        
    }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {

    }
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_current_term;
    uint64_t m_prev_log_idx;
    uint64_t m_prev_log_term;
    uint64_t m_leader_commit_idx;
    std::vector<log_entry> m_entries;
};


struct append_entry_accepted {
    uint64_t last_new_idx;
};
struct append_entry_rejected {
    uint64_t non_matching_idx;
    uint64_t last_idx;
};

class append_entry_reply : public task {
public:
    std::shared_ptr<transport::frame> gen_frame() override {
        
    }
    co_context::task<void> callback(std::shared_ptr<transport::frame> reply_frame) override {

    }
private:
    uint64_t m_from_id;
    uint64_t m_group_id;
    uint64_t m_term;
    uint64_t m_index;
    std::variant<append_entry_accepted, append_entry_rejected> m_result;

};
}