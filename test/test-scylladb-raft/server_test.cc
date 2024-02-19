

#include "server_test.hh"
#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "helpers.hh"
BOOST_AUTO_TEST_CASE(server_01) {
    co_context::io_context ctx;
    // ctx.co_spawn([]() -> co_context::task<> {
    //     // std::cout << "co_spawn" << std::endl;
    //     environment env;
    
    //     co_await env.call();
    //     auto id1 = co_await env.new_server();
    //     auto id2 = co_await env.new_server();
    //     auto id3 = co_await env.new_server();

    //     auto leader_id = id1;
    //     // co_await server.add_entry()
    //     // co_await server.read_barrier()
    //     // co_await read the value
    //     for (int i = 0; i < 100; i++) {
    //         co_await env.call();  
    //     };

    //     co_return;
    // }());
    ctx.co_spawn([]() -> co_context::task<> {
        cluster cls(3);

        cls.
        co_return;
    }());
    ctx.start();
    ctx.join();
}