#include <co_context/io_context.hpp>
#include <iostream>

#include "karma-service/config.h"
#include "karma-service/worker.h"
#include "toml.hpp"
int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    co_context::io_context ctx;
    auto config = toml::parse_file("/home/tpa/Heimerdinger-Lab/Karma/config/karma-" +
                                   std::string(argv[1]) + ".toml");
    std::cout << "/home/tpa/Heimerdinger-Lab/Karma/config/karma-" + std::string(argv[1]) + ".toml"
              << std::endl;
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
