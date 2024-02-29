#include <memory>
#include <string>
#define BOOST_TEST_MODULE KARMA_RAFT_TEST
#include "server_test2.hh"
#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "helpers.hh"
#include <boost/test/unit_test.hpp>
// #define BOOST_TEST_MODULE KARMA_RAFT_TEST



BOOST_AUTO_TEST_CASE(server_01) {
    co_context::io_context ctx;
    ctx.co_spawn([]() -> co_context::task<> {
        auto cls = std::make_shared<cluster>(3);
        co_await cls->start();
        std::cout << " 0 " << std::endl;
        for (int i = 0; i < 30; i++) {
            std::cout << i << std::endl;
            co_await co_context::timeout(std::chrono::seconds{1});
            co_await cls->tick();
        }
        co_spawn([](std::shared_ptr<cluster> ptr) -> co_context::task<> {
            int cnt = 0;
            while (1) {
                co_await co_context::timeout(std::chrono::seconds{1});
                // co_await co_context::timeout(1s);
                co_await ptr->tick();
                std::cout << "cnt = " << ++cnt <<  ", leader: " << ptr->who_is_leader() << std::endl;
            }
            // for (int i = 1; i <= 1e5; i++) {
            //     co_await ptr->tick();
            // }
        }(cls));
        std::cout << " 1 " << std::endl;
        auto svr1 = cls->get_raft_server(1);
        auto svr2 = cls->get_raft_server(2);
        auto svr3 = cls->get_raft_server(3);
        std::cout << "svr1.leader " << svr1->is_leader() << std::endl;
        std::cout << "svr2.leader " << svr2->is_leader() << std::endl;
        std::cout << "svr3.leader " << svr3->is_leader() << std::endl;
        std::cout << " 2 " << std::endl;
        co_await svr1->put("nihao", "wock");
        std::cout << "svr1.leader " << svr1->is_leader() << std::endl;
        // // std::cout << "svr2.leader " << svr2->is_leader() << std::endl;
        // // std::cout << "svr3.leader " << svr3->is_leader() << std::endl;
        std::cout << " 3 " << std::endl;
        auto value = co_await svr1->get("nihao");
        std::cout << "value ===" << value << std::endl;
        std::cout << " 4 " << std::endl;
        for (int i = 1; i <= 100; i++) {
            co_await svr1->put("abc", std::to_string(i));
            auto value = co_await svr1->get("abc");
            // assert(value, std::to_string(i));
            BOOST_CHECK(value.compare(std::to_string(i)) == 0);
            std::cout << "abc === " << value << std::endl;
        }
        while(1) {
            co_await co_context::yield();
        }
        co_return;
    }());
    ctx.start();
    ctx.join();
}