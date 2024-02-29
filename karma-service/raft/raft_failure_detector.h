#pragma once
#include "karma-raft/raft.hh"
namespace service {
class raft_failure_detector : public raft::failure_detector {
   public:
    ~raft_failure_detector() {}
    // Called by each server on each tick, which defaults to 10
    // per second. Should return true if the server is
    // alive. False results may impact liveness.
    bool is_alive(raft::server_id server) { return true; }
};
}  // namespace service
