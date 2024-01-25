#pragma once
#include <memory>
#include <string>
namespace store {
class db {
public:
    db() {
        // 创建indexer（内存中，暂时用map）
        // std::thread
        // 创建thread
        // 创建io（将indexr丢进去）
        // io.run()
            // complete_write_tasks，修改index
    }
    static std::shared_ptr<db> open(std::string file_path);
    void put(std::string key, std::string value);
    void close();
    void get(std::string key);
    void deletde(std::string key);

private:
    // indexer
    // store
};
};