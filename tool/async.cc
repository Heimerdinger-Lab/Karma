#include "co_context/all.hpp"
#include "co_context/co/channel.hpp"
#include "co_context/co/condition_variable.hpp"
#include "co_context/co/mutex.hpp"
#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "co_context/task.hpp"
#include <iostream>
#include <memory>
#include <variant>
using namespace std::chrono_literals;
struct Result {
    std::string m_str;
};
struct Task {
    std::string m_str;
    std::shared_ptr<co_context::channel<std::monostate, 0>> m_channel;
};
// co_context::lazy<void> func() {

// };
class Demo {
public:
    Demo() {
        printf("%10s at \t%lx\n", "Demo", uintptr_t(&co_context::this_io_context()));
        co_context::co_spawn(loop());
    }
    co_context::task<void>  Print(std::string str) {
        printf("%10s at \t%lx\n", "Print", uintptr_t(&co_context::this_io_context()));
        Task task;
        task.m_str = str;
        // auto observer = std::make_shared<co_context::channel<Result, 0>>();
        auto observer = std::make_shared<co_context::channel<std::monostate, 0>>();
        task.m_channel = observer;
        
        co_await m_channel.release(task);
        using namespace std::literals;

        // auto result = co_await co_context::timeout(observer->acquire(), 3s);
        std::cout << "i receive the result. " << std::endl;
        printf("%10s at \t%lx\n", "Print result", uintptr_t(&co_context::this_io_context()));
    }
private:
    co_context::task<void> loop() {
        while(1) {
            auto task = co_await m_channel.acquire();
            std::cout << "receive 1 task: " << task.m_str << std::endl;
            printf("%10s at \t%lx\n", "Loop", uintptr_t(&co_context::this_io_context()));
            co_await co_context::lazy::timeout(1s);
            // Result result;
            // result.m_str = "I am the solution";
            co_await task.m_channel->release(std::monostate());
        }
    }
    co_context::channel<Task> m_channel;
};
// Demo* g_demo = nullptr;
std::shared_ptr<Demo> g_demo;
co_context::task<void> func() {
    std::shared_ptr<Demo> demo = std::make_shared<Demo>();
    g_demo = demo;
    while(1) {
        co_await co_context::lazy::timeout(1s);
        // co_await demo.Print("Hello I am Tianpingan");
    }
}
co_context::task<void> func2() {
    printf("%10s at \t%lx\n", "func2", uintptr_t(&co_context::this_io_context()));
    while (g_demo.get() == nullptr) {
        co_await co_context::lazy::timeout(1s);
    }
    co_await g_demo->Print("Hello from the other side");
}
int main() {
    std::thread t1([](){
        co_context::io_context ctx;
        ctx.co_spawn(func());
        ctx.start();
        ctx.join();
    });

    std::thread t2([](){
        co_context::io_context ctx;
        ctx.co_spawn(func2());
        ctx.start();
        ctx.join();
    });
    t1.join();
    t2.join();
    return 0;

}
