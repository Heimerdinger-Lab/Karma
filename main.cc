#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "co_context/log/log.hpp"
#include <boost/property_tree/ini_parser.hpp>
#include <coroutine>
#include <iostream>
#include <memory>
#include "karma-service/raft/raft_failure_detector.h"
#include "karma-service/raft/raft_persistence.h"
#include "karma-service/raft/raft_rpc.h"
#include "karma-service/raft/raft_state_machine.h"
#include "scylladb-raft/server.hh"
class config {
public:
    config() {}
    std::string addr = "127.0.0.1";
    uint16_t port = 8888;
    uint8_t core_num = 8;
    uint8_t raft_gourp_count = 1;
};
void on_get() {

}
void on_put() {

}
co_context::task<> func() {
    config demo_config;
    std::cout << "in main" << std::endl;
    co_context::log::d("karma is starting...");
    std::unique_ptr<raft_state_machine> sm = std::unique_ptr<raft_state_machine>();
    auto& rsm = *sm;

    std::unique_ptr<raft_rpc> rpc = std::unique_ptr<raft_rpc>();
    std::unique_ptr<raft_persistence> ps = std::unique_ptr<raft_persistence>();
    std::shared_ptr<raft_failure_detector> rfd = std::shared_ptr<raft_failure_detector>();
    
    raft::server::configuration cfg;
    std::unique_ptr<raft::server> svr = raft::create_server(0, std::move(rpc), std::move(sm), std::move(ps), rfd, cfg);
    
    // raft::command cmd("0123456789");
    // co_await svr->add_entry(cmd, raft::wait_type::committed);

    // co_await svr->read_barrier();

    // auto value = rsm.get("key");

    co_return;
}
int main() {
    co_context::io_context ctx;
    ctx.co_spawn(func());
    ctx.start();
    ctx.join();
    return 0;
}

// using namespace co_context;

// task<> cycle(int sec, const char *message) {
//     while (true) {
//         co_await timeout(std::chrono::seconds{sec});
//         printf("%s\n", message);
//     }
// }

// task<> cycle_abs(int sec, const char *message) {
//     auto next = std::chrono::steady_clock::now();
//     while (true) {
//         next = next + std::chrono::seconds{sec};
//         co_await timeout_at(next);
//         printf("%s\n", message);
//     }
// }

// int main() {
//     io_context ctx;
//     ctx.co_spawn(cycle(1, "1 sec"));
//     ctx.co_spawn(cycle_abs(1, "1 sec [abs]"));
//     ctx.co_spawn(cycle(3, "\t3 sec"));
//     ctx.start();
//     ctx.join();
//     return 0;
// }