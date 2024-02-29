// #include "co_context/io_context.hpp"
// #include "co_context/lazy_io.hpp"
// #include "co_context/log/log.hpp"
// #include <boost/property_tree/ini_parser.hpp>
// #include <coroutine>
// #include <iostream>
// #include <memory>
// #include "karma-service/raft/raft_failure_detector.h"
// #include "karma-service/raft/raft_persistence.h"
// #include "karma-service/raft/raft_rpc.h"
// #include "karma-service/raft/raft_state_machine.h"
// #include "karma-raft/server.hh"
// #include "karma-raft/simple_server.hh"
// #include "temp.h"
// class config {
// public:
//     config() {}
//     std::string addr = "127.0.0.1";
//     uint16_t port = 8888;
//     uint8_t core_num = 8;
//     uint8_t raft_gourp_count = 1;
// };
// void on_get() {

// }
// void on_put() {

// }
// co_context::task<> func() {
//     config demo_config;
//     std::cout << "in main" << std::endl;
//     co_context::log::d("karma is starting...");
//     std::unique_ptr<raft_state_machine> sm = std::unique_ptr<raft_state_machine>();
//     auto& rsm = *sm;

//     std::unique_ptr<raft_rpc> rpc = std::unique_ptr<raft_rpc>();
//     std::unique_ptr<raft_persistence> ps = std::unique_ptr<raft_persistence>();
//     std::unique_ptr<raft_failure_detector> rfd = std::unique_ptr<raft_failure_detector>();
    
//     // auto svr = sb_server::create(0, std::move(sm), std::move(rpc), std::move(rfd));
//     // auto svr = raft_server::create(1)
//     // svr->receive(raft::server_id from, raft::rpc_message msg)

//     // raft::server::configuration cfg;
//     // std::unique_ptr<raft::server> svr = raft::create_server(0, std::move(rpc), std::move(sm), std::move(ps), rfd, cfg);
    

//     // raft::command cmd("0123456789");
//     // co_await svr->add_entry(cmd, raft::wait_type::committed);

//     // co_await svr->read_barrier();

//     // auto value = rsm.get("key");

//     co_return;
// }
// int main() {
//     co_context::io_context ctx;
//     ctx.co_spawn(func());
//     ctx.start();
//     ctx.join();
//     return 0;
// }
#include "karma-service/config.h"
#include "toml.hpp"
#include "karma-service/worker.h"
#include <co_context/io_context.hpp>
#include <iostream>
int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    co_context::io_context ctx;
    auto config = toml::parse_file( "/home/tpa/Heimerdinger-Lab/Karma/config/karma-" + std::string(argv[1]) + ".toml");
    std::cout << "/home/tpa/Heimerdinger-Lab/Karma/config/karma-" + std::string(argv[1]) + ".toml" << std::endl;
    service::config cfg;
    cfg.m_listen_ip = config["server"]["listen_addr"].value_or("0.0.0.0");
    cfg.m_listen_port = config["server"]["listen_port"].value_or(8888);
    cfg.m_id = config["raft"]["server_id"].value_or(5555);
    cfg.m_count = config["raft"]["count"].value_or(3);
    auto members = config["raft"]["members"];
    // std::cout << "members.size: " << members->size() << std::endl;
    std::cout << "current: " << cfg.m_id << std::endl;
    std::cout << "listen_addr: " << cfg.m_listen_ip << ":" << cfg.m_listen_port << std::endl;
    for (int i = 0; i < config["raft"]["count"].value_or(5); i++) {
        std::cout << "member[" << i << "]: " << members[i] << std::endl;
        cfg.m_members[i + 1] = members[i].value_or("value");
    }
    ctx.co_spawn([cfg]() -> co_context::task<> {
        service::worker wk(cfg);
        co_await wk.start();
        co_await wk.loop();
    }());
    ctx.start();
    ctx.join();
    return 0;
}

