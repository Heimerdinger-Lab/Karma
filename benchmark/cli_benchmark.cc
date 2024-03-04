// #include <boost/log/core.hpp>
// #include <boost/log/expressions.hpp>
// #include <boost/log/trivial.hpp>
// #include <string>

// #include "co_context/io_context.hpp"
// #include "karma-client/client.h"
// co_context::task<> cli() {
//     boost::log::core::get()->set_filter(boost::log::trivial::severity >=
//                                         boost::log::trivial::fatal);
//     int MAXN = 5000;
//     std::map<uint64_t, std::string> members;
//     members[1] = "127.0.0.1:8888";
//     client::client ccc(members);
//     auto start = std::chrono::steady_clock::now();
//     for (int i = 0; i < MAXN; i++) {
//         if (i % 1000 == 0) {
//             std::cout << "i = " << i << std::endl;
//         }
//         auto reply = co_await ccc.cli_echo(0, 1, "ping");
//     }
//     auto end = std::chrono::steady_clock::now();
//     auto diff = end - start;
//     double elapsedTime = std::chrono::duration<double, std::milli>(diff).count();
//     std::cout << "Elapsed time: " << elapsedTime << " milliseconds" << std::endl;
//     std::cout << "QPS: " << (MAXN) / (elapsedTime / 1000) << std::endl;
// }
// int main() {
//     co_context::io_context ctx[10];
//     for (int i = 0; i < 1; i++) {
//         for (int j = 0; j < 11; j++) {
//             ctx[i].co_spawn(cli());
//         }
//     }
//     for (int i = 0; i < 10; i++) {
//         ctx[i].start();
//     }
//     for (int i = 0; i < 10; i++) {
//         ctx[i].join();
//     }
// }