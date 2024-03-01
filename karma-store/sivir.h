#pragma once

#include <flatbuffers/flatbuffer_builder.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <set>
#include <string>

#include "co_context/co/channel.hpp"
#include "co_context/co/condition_variable.hpp"
#include "co_context/io_context.hpp"
#include "co_context/lazy_io.hpp"
#include "karma-cache/s3fifo.h"
#include "karma-store/buf/aligned_buf.h"
#include "karma-store/buf/aligned_buf_reader.h"
#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-util/sslice.h"
#include "options.h"
#include "protocol/rpc_generated.h"
#include "wal.h"
#include "write_window.h"
// #include <liburing/io_uring.h>
#include <liburing.h>

#include <thread>
#include <utility>
#include <variant>
#include <vector>

class sivir : public std::enable_shared_from_this<sivir> {
   public:
    sivir(open_options &opt);
    void start();
    ~sivir() {
        std::cout << "dead !" << std::endl;
        // m_worker.join();
        m_ctx->join();
    }
    void recovery() {}
    co_context::task<> put(const write_options &opt, std::string key, std::string value);
    co_context::task<> del(std::string key);
    co_context::task<> get(std::string key, std::string *value);

   public:
    /*
        1. 读请求
            1.1 m_buf表示读到的数据
            1.2 read_task 表示该sqe对应的read_task
        2. 写请求
            2.1 m_buf表示要写的数据


        reap 写请求后，更新write_window，然后再更新commit
       index，然后再给相关的write_task的observer发送数据

        reap 读请求后，直接给read_task的observer发送数据
    */
    struct context {
        // sqe 的 user_data
        uint8_t m_opcode;
        // 写和读的数据
        std::shared_ptr<aligned_buf> m_buf;
        // 写的情况下，m_wal_offset是m_buf的wal_offset
        // 读的情况下，就是要读的位置
        uint64_t m_wal_offset;
        uint64_t m_len;

        // 读的情况
        uint32_t m_read_idx;

        // 对于写
        //  commit(m_buf.wal_offset, m_buf.wal_offset + aligned_buf.limit());

        // 对于读
        // 拷贝[m_wal_offset, m_len)到read_result
    };
    struct write_result {
        // record的起始偏移
        // record的长度
        uint64_t m_wal_offset;
        uint32_t m_size;
    };
    struct write_task {
        sslice m_data;
        // observer
        // std::shared_ptr<std::queue<write_result>> m_channel;
        // std::shared_ptr<std::promise<write_result>> m_prom;

        co_context::channel<write_result> &m_prom;
        // std::shared_ptr<co_context::io_context> m_ctx;
        co_context::io_context &m_ctx;
    };

    struct read_result {
        // [wal_offset, wal_offset + size]
        // sslice m_value;
    };
    struct read_task {
        uint64_t m_wal_offset;
        uint32_t m_len;
        std::string *m_value;
        // observer
        // std::queue<read_result> m_channel;
        // std::shared_ptr<std::promise<read_result>> m_prom;
        co_context::channel<read_result> &m_prom;
        co_context::io_context &m_ctx;
    };
    struct record_pos {
        uint64_t wal_offset;
        uint64_t record_size;
    };
    void init_uring() {}
    co_context::task<> worker_run();
    co_context::task<int> on_complete(context *ctx);
    co_context::task<> complete_write_task();
    co_context::task<> complete_read_task(uint32_t idx, context *ctx);
    co_context::task<> reap_data_tasks();
    int receive_io_tasks();
    void build_write_sqe();
    void build_sqe();
    co_context::task<record_pos> add_record(std::string record);
    co_context::task<> get_record(uint64_t wal_offset, uint64_t size, std::string *value);
    void compactor_run() {
        // gc_wal_offset 表示之前的wal都是垃圾，可以回收了
        // 一个segment能不能删，就看它的wal_offset + size 是否小于 gc_wal_offset
        // 如果小于，说明可以

        // compactor就是不断更新的
    }
    void check_and_allocated() {
        // check当前预分配的文件
        // 一般要提前分配两个
    }

    // private:
   public:
    io_uring m_data_ring;
    //
    std::map<std::string, record_pos> m_map;
    //
    // std::map<std::string, std::string> m_cached;
    cache::s3fifo m_cached;
    // std::thread m_worker;
    std::thread m_compactor;
    write_window m_window;
    // 从channel中获取得到的io任务
    std::vector<std::variant<write_task *, read_task *>> m_pending_io_tasks;
    // key是cursor，value是对应的任务
    // 当commit index超过cursor时，说明对应的任务已经完成
    // 可以给inflight_write_task的observer发送响应信息了
    std::map<uint64_t, write_task *> m_inflight_write_tasks;

    // key是read_task的编号
    std::map<uint32_t, read_task *> m_inflight_read_tasks;
    // key是这个buf的wal_offset
    // [key, key + align] 正在inflight
    std::set<uint64_t> m_barrier;
    std::unique_ptr<co_context::io_context> m_ctx;

    // 这些是阻塞的
    // buf的wal_offset
    // std::map<uint64_t, std::pair<context, io_uring_sqe>> m_blocked;

    wal m_wal;
    std::unique_ptr<aligned_buf_writer> m_buf_writer;
    std::vector<io_uring_sqe> m_resubmit_sqes;
    // std::mutex m_mtx;
    std::queue<std::variant<write_task *, read_task *>> m_channel;
};
