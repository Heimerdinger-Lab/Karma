#include <string>

#include "co_context/io_context.hpp"
#include "karma-client/client.h"
#include "karma-raft/raft.hh"
// co_context::task<> cli() {
//     std::map<uint64_t, std::string> members;
//     members[0] = "127.0.0.1:5555";
//     members[1] = "127.0.0.1:8888";
//     client::client ccc(members);
//     // raft::server_address from(0, "127.0.0.1:5555");
//     // raft::server_address target(1, "127.0.0.1:8888");
//     for (int i = 0; i < 1024 * 1024 ; i++) {
//         auto reply = co_await ccc.echo(0, 1, "Hello From Tianpingan, idx: " +
//         std::to_string(i)); std::cout << "cli receive the reply: " <<
//         reply->msg() << std::endl;
//     }
// }
co_context::task<> cli() {
    std::map<uint64_t, std::string> members;
    // 这是中央钦点的leader！！！
    members[1] = "127.0.0.1:8888";
    client::client ccc(members);
    // raft::server_address from(0, "127.0.0.1:5555");
    while (true) {
        std::cout << "1: write" << std::endl;
        std::cout << "2: read" << std::endl;
        int opt;
        std::cin >> opt;
        if (opt == 1) {
            co_await ccc.cli_write(0, 1, "key01", "value01");
        } else if (opt == 2) {
            auto value = co_await ccc.cli_read(0, 1, "key01");
            std::cout << "value = " << value->value() << std::endl;
        } else {
            break;
        }
    }
}
int main() {
    co_context::io_context ctx;
    ctx.co_spawn(cli());
    ctx.start();
    ctx.join();
}