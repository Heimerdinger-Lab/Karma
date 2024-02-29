#include "co_context/io_context.hpp"
#include "karma-store/sivir.h"
#include <memory>
#include <random>
#include <string>
std::string generateRandomString(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(97, 122); // ASCII码中小写字母a-z的范围

    std::string str;
    for (int i = 0; i < length; ++i) {
        str += static_cast<char>(dis(gen)); // 将随机生成的整数转换为字符
    }
    return str;
}
int main() {
    // 外部的
    // 线程数
    // 吞吐
    // key就是index
    // value大小可以设置
    // 顺序读，随机读，有倾向读
    // 总的数据量
    // QPS，延时（平均，p99）
    open_options config;
    config.queue_depth = 32768;
    config.sqpoll_cpu = 0;
    config.path = "/home/tpa/Heimerdinger-Lab/Karma/temp";
    // sivir db(config);
    auto db_ptr = std::make_shared<sivir>(config);
    db_ptr->start();
    co_context::io_context ctx;
    ctx.co_spawn([](std::shared_ptr<sivir> db) -> co_context::task<> {
        // co_await db->start();

        for (int i = 0; i < 1024; i++) {
            const write_options opt;
            auto val = generateRandomString(128);
            co_await db->put(opt, std::to_string(i), val);
            std::string value;
            co_await db->get(std::to_string(i), &value);
            std::cout << i<< ": val : " << val << ", value:  " << value << std::endl;
            assert(val.compare(value) == 0);
            // co_return;
        }
    }(db_ptr));
    ctx.start();
    ctx.join();
}