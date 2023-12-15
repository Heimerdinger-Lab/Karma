#include "helper.h"

raft::snapshot_descriptor log_snapshot(raft::log& log, raft::index_t idx) {
    return raft::snapshot_descriptor{.idx = idx, .term = log.last_term(), .config = log.get_snapshot().config};
}

raft::config_member_set config_set(std::vector<raft::server_id> ids) {
    raft::config_member_set set;
    for (auto id : ids) {
        set.emplace(config_member_from_id(id));
    }
    return set;
}
raft::server_address server_addr_from_id(raft::server_id id) {
    return raft::server_address{id, {}, {}};
}
raft::config_member config_member_from_id(raft::server_id id) {
    return raft::config_member{server_addr_from_id(id), true};
}
raft::configuration config_from_ids(std::vector<raft::server_id> ids) {
    return raft::configuration{config_set(std::move(ids))};   
}
void election_timeout(raft::fsm& fsm) {
    for (int i = 0; i <= 2 * raft::ELECTION_TIMEOUT.count(); i++) {
        fsm.tick();
    }
}
raft::fsm_config fsm_cfg{.append_request_threshold = 1, .enable_prevoting = false};
raft::fsm_config fsm_cfg_pre{.append_request_threshold = 1, .enable_prevoting = true};
struct trivial_failure_detector trivial_failure_detector;
