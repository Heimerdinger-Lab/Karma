#include "co_context/net/socket.hpp"
#include "co_context/task.hpp"
#include "karma-client/client.h"
#include "karma-raft/rpc_message.h"
#include "karma-service/session.h"
#include <cstdio>
#include <memory>
#include <ostream>
#include <variant>
#include <vector>
#define BOOST_TEST_MODULE KARMA_CLIENT_TEST
#include <boost/test/unit_test.hpp>
namespace test {
    BOOST_AUTO_TEST_SUITE (ClientTest)
        co_context::task<void> client() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            client::client cli;
            raft::server_address start(0, "127.0.0.1", 8888);
            raft::server_address target(0, "127.0.0.1", 8889);
            raft::ping_pong_request req{.m_msg = "Hello from tianpingan"};
            auto ret = co_await cli.ping_pong(start, target, req);
            if (std::holds_alternative<client::client_error>(ret)) {
                // rpc失败
            } else {
                auto reply = std::get<raft::ping_pong_reply>(ret);
                // rpc成功，得到reply
                std::cout << "size = " << reply.m_msg.size() << std::endl;
                std::cout << "I receive " <<  reply.m_msg << std::endl;
                // printf("reply: %s\n", reply.m_msg.c_str());
            }
            std::cout << " ======= " << std::endl;
            raft::ping_pong_request req2{.m_msg = "Hello from tianpingan2"};
            auto ret1 = co_await cli.ping_pong(start, target, req2);
            if (std::holds_alternative<client::client_error>(ret1)) {
                // rpc失败
            } else {
                auto reply = std::get<raft::ping_pong_reply>(ret1);
                // rpc成功，得到reply
                std::cout << "size = " << reply.m_msg.size() << std::endl;
                std::cout << reply.m_msg << std::endl;
                // printf("reply: %s\n", reply.m_msg.c_str());
            }
        }
        co_context::task<void> server(const uint16_t port) {
            co_context::acceptor ac{co_context::inet_address{port}};
            std::cout << "start to listen at " << port << std::endl;
            std::vector<std::shared_ptr<service::session>> sessions;
            for (int sock; (sock = co_await ac.accept()) >= 0;) {
                // co_context::socket sb{sock};
                auto sb = std::make_shared<co_context::socket>(sock);
                auto ts = std::make_shared<service::session>(sb, std::string("127.0.0.1"), port);
                // ts->process();
                sessions.push_back(ts);
                ts->process();
            }
        }
        co_context::task<> func() {
            auto ctx = std::make_shared<co_context::task<int>>();
            std::cout << "!" << std::endl;
            co_await *ctx;
            std::cout << "2" << std::endl;
        }
        BOOST_AUTO_TEST_CASE(Test01) {
            // co_context::io_context ctx1, ctx2;
            // ctx1.co_spawn(server(8889));
            // ctx2.co_spawn(client());
            // ctx1.start();
            // ctx2.start();
            // ctx2.join();
        }
        BOOST_AUTO_TEST_CASE(Test02) {
            co_context::io_context ctx;
            ctx.co_spawn(func());
            ctx.start();
            ctx.join();
        }
    BOOST_AUTO_TEST_SUITE_END()
} // namespace test