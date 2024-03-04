#include <memory>
#include <random>
#include <string>

#include "co_context/io_context.hpp"
#include "karma-store/sivir.h"
std::string generateRandomString(int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(97, 122);  // ASCII码中小写字母a-z的范围

    std::string str;
    for (int i = 0; i < length; ++i) {
        str += static_cast<char>(dis(gen));  // 将随机生成的整数转换为字符
    }
    return str;
}
int main() {
    /*
        notes: 在context里面不能再创建context?
    */
    // co_context::io_context ctx;

    auto db_ptr = std::make_shared<store::sivir>();
    // db_ptr->start();
    open_options config;
    config.queue_depth = 32768;
    config.sqpoll_cpu = 0;
    config.segment_file_size = 1024 * 1024 * 1024;
    config.path = "/home/tpa/Heimerdinger-Lab/Karma/temp/";
    db_ptr->open(config);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    co_context::io_context ctx;
    ctx.co_spawn([](std::shared_ptr<store::sivir> db_ptr) -> co_context::task<> {
        uint32_t TOTAL_BYTES = 1024 * 1024 * 1024 * 2;
        uint32_t IO_SIZE = 4096 * 1024;
        uint32_t COUNT = TOTAL_BYTES / IO_SIZE;
        auto val = generateRandomString(IO_SIZE);
        auto start = std::chrono::steady_clock::now();
        std::string key = std::to_string(0);
        for (uint32_t i = 0; i < COUNT; i++) {
            const write_options opt;
            co_await db_ptr->put(opt, key, val);
            if (i % 1024 == 0) {
                std::cout << "i: " << i << std::endl;
            }
        }
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        double elapsedTime = std::chrono::duration<double, std::milli>(diff).count();
        std::cout << "Elapsed time: " << elapsedTime << " milliseconds" << std::endl;
        std::cout << "Output: " << (TOTAL_BYTES / 1024 / 1024) / (elapsedTime / 1000) << "MB/S"
                  << std::endl;
    }(db_ptr));
    ctx.start();
    ctx.join();
}