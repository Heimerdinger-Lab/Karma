#include "co_context/io_context.hpp"
#include "karma-client/client.h"
#include "karma-raft/raft.hh"
#include <string>
co_context::task<> cli() {
    std::map<uint64_t, std::string> members;
    members[0] = "127.0.0.1:6666";
    client::client ccc(members);
    raft::server_address from(1, "127.0.0.1:6666");
    raft::server_address target(1, "127.0.0.1:6666");
    for (int i = 0; i < 1024 * 1024 ; i++) {
        auto reply = co_await ccc.echo(from, target, "Hello From Tianpingan, idx: " + std::to_string(i));
        std::cout << "cli receive the reply: " << reply->msg() << std::endl;
    }
}
int main() {
    co_context::io_context ctx;
    ctx.co_spawn(cli());
    ctx.start();
    ctx.join();
}