#pragma once
#include "karma-raft/common.h"
#include "karma-raft/fsm.h"
#include "karma-raft/log.h"
#include <memory>
void election_timeout(raft::fsm& fsm);
template <typename T> void add_entry(raft::log& log, T cmd) {
    log.emplace_back(std::make_shared<raft::log_entry>(raft::log_entry{log.last_term(), log.next_idx(), cmd}));
}
inline raft::server_id id() {
    static int id = 0;
    return raft::server_id(++id);
}
raft::snapshot_descriptor log_snapshot(raft::log& log, raft::index_t idx);
raft::config_member_set config_set(std::vector<raft::server_id> ids);
raft::server_address server_addr_from_id(raft::server_id);
raft::config_member config_member_from_id(raft::server_id);
raft::configuration config_from_ids(std::vector<raft::server_id> ids);

struct trivial_failure_detector: public raft::failure_detector {
    bool is_alive(raft::server_id from) override {
        return true;
    }
};
extern struct trivial_failure_detector trivial_failure_detector;
extern raft::fsm_config fsm_cfg;
extern raft::fsm_config fsm_cfg_pre;
// fsm_debug create_follower(raft::server_id id, raft::log log,
//         raft::failure_detector& fd = trivial_failure_detector);
