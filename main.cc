
#include <boost/log/trivial.hpp>
#include <co_context/io_context.hpp>

#include "karma-service/config.h"
#include "karma-service/worker.h"
#include "toml.hpp"
service::config parse_config(std::string path) {
    auto config = toml::parse_file(path);
    service::config cfg;
    cfg.m_id = config["raft"]["server_id"].value_or(5555);
    cfg.m_count = config["raft"]["count"].value_or(3);
    cfg.m_store_path = config["store"]["path"].value_or("~/temp/");
    // cfg.m_listen_ip =
    auto members = config["raft"]["members"];
    for (int i = 0; i < config["raft"]["count"].value_or(5); i++) {
        BOOST_LOG_TRIVIAL(debug) << "member[" << i << "]: " << members[i];
        cfg.m_members[i + 1] = members[i].value_or("value");
    }
    return cfg;
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        BOOST_LOG_TRIVIAL(error) << "./karma <configuration file path>";
        return 0;
    }
    co_context::io_context ctx;
    BOOST_LOG_TRIVIAL(debug) << "Start Genshin Impact!";
    service::config cfg = parse_config(argv[1]);
    ctx.co_spawn([cfg]() -> co_context::task<> {
        service::worker wk(cfg);
        co_await wk.start();
        co_await wk.loop();
    }());
    ctx.start();
    ctx.join();

}
