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
#include "co_context/co/mutex.hpp"
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

#include <boost/noncopyable.hpp>
#include <variant>
#include <vector>

#include "common.h"
namespace store {
class sivir : public std::enable_shared_from_this<sivir>, public boost::noncopyable {
   public:
    sivir();
    bool open(open_options &opt);
    ~sivir() { m_io_ctx->join(); }
    co_context::task<bool> put(const write_options &opt, std::string &key, std::string &value);
    co_context::task<bool> del(std::string &key);
    co_context::task<bool> get(std::string &key, std::string &value);

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
    bool decord_record(std::string &record, std::string &key, std::string &value);
    void init_uring() {}
    co_context::task<> worker_run();
    co_context::task<bool> on_complete(uring_context *ctx);
    co_context::task<> complete_write_task();
    co_context::task<> complete_read_task(uint64_t idx, read_uring_context *ctx);
    co_context::task<> reap_data_tasks();
    co_context::task<int> receive_io_tasks();
    void build_write_sqe();
    void build_sqe();
    co_context::task<std::optional<record_pos>> add_record(std::string &record);
    co_context::task<bool> get_record(uint64_t wal_offset, uint64_t size, std::string &value);
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
    open_options m_opt;
    io_uring m_data_ring;
    std::map<std::string, record_pos> m_map;
    cache::s3fifo m_cached;
    write_window m_window;
    wal m_wal;
    std::unique_ptr<co_context::io_context> m_io_ctx;
    std::unique_ptr<aligned_buf_writer> m_buf_writer;
    // 从channel中获取得到的io任务
    std::vector<std::variant<write_io *, read_io *>> m_pending_io_tasks;

    // key是cursor，value是对应的任务
    // 当commit index超过cursor时，说明对应的任务已经完成
    // 可以给inflight_write_task的observer发送响应信息了
    std::map<uint64_t, write_io *> m_inflight_write_tasks;
    // key是read_task的编号
    std::map<uint64_t, read_io *> m_inflight_read_tasks;

    // key是这个buf的wal_offset
    // [key, key + align] 正在inflight
    std::set<uint64_t> m_barrier;

    // 这些是阻塞的
    // buf的wal_offset
    // std::map<uint64_t, std::pair<context, io_uring_sqe>> m_blocked;

    std::vector<io_uring_sqe> m_resubmit_sqes;
    // std::mutex m_mtx;
    // TODO: Find a better method
    std::mutex m_mtx;
    // co_context::mutex m_mtx;
    std::queue<std::variant<write_io *, read_io *>> m_channel;
    // co_context::channel<std::variant<write_io *, read_io *>, 1024> m_channel;
    uint64_t m_read_seq;
};

}  // namespace store
