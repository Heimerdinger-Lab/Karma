#pragma once
#include "karma-store/buf/aligned_buf.h"
#include "karma-store/buf/aligned_buf_reader.h"
#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-store/segment_file.h"
#include "karma-util/sslice.h"
#include "options.h"
#include "wal.h"
#include "write_window.h"
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <liburing.h>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
class sivir {
public:
    sivir(open_options& opt) {
        // 1. 初始化io_uring，开启SQPOLL和IOPOLL
        io_uring_params params;
        params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
        params.sq_thread_cpu = opt.sqpoll_cpu;
        io_uring_queue_init_params(opt.queue_depth, &m_data_ring, &params);
        // io_uring_queue_init(opt.queue_depth, &m_data_ring, params.flags);
        // 2. 打开目录下的wal
        m_wal.load_from_path(opt.path);

        // 3. 不断读wal的下一条record，读出record解析，更新memtable
        // record = key + value + wal_offset
        sslice record;
        uint64_t wal_offset = 0;
        uint64_t current_offset = 0;
        while(m_wal.scan_record(wal_offset, &record)) {
            //
            sslice key, value;
            // decord_record(record, key, value);
            m_map[key.ToString()] = record_pos {
                .wal_offset = current_offset,
                .m_value = value,
            };
            current_offset = wal_offset;
        };
        // m_wal.try_open_segment();
        // 4. 创建工作线程
        m_buf_writer = std::make_shared<aligned_buf_writer>(0);
        m_worker = std::thread(&sivir::worker_run, this);
        // m_worker.join();
    };
    ~sivir() {

    }
    void recovery() {

    }
    void put(const write_options& opt, std::string key, std::string value) {
        // 调用add_record写wal
        // 得到record_pos
        // 更新memtable和cache
        auto pos = add_record("123");

        // update the memtable and the cached

    }
    void del(std::string key) {
        // 
    }
    void get(std::string key, std::string *value) {

        // check the cached
        // search the memtable 
        // lookup in the wal
    }
public:
    /*
        1. 读请求
            1.1 m_buf表示读到的数据
            1.2 read_task 表示该sqe对应的read_task
        2. 写请求
            2.1 m_buf表示要写的数据
        

        reap 写请求后，更新write_window，然后再更新commit index，然后再给相关的write_task的observer发送数据

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
        std::shared_ptr<std::promise<write_result>> m_prom;
    };

    struct read_result {
        uint64_t m_wal_offset;
        sslice m_buf;
    };
    struct read_task {
        uint64_t m_wal_offset;
        uint32_t m_len;
        // observer
        std::queue<read_result> m_channel;
    };
    struct record_pos {
        uint64_t wal_offset;
        sslice m_value;
    };
    void init_uring() {}
    void worker_run() {
        // loop
        // 对于写io_task
        //  1. 放入alignbuffer
        // 待所有都放完了，按照alignbuffer去生产sqe
        // 对于读io_task
        //  1. 读alignbuffer？
        std::cout << "worker_run" << std::endl;
        while(true) {
            m_wal.try_open_segment();
            m_wal.try_close_segment(0);
            int cnt = receive_io_tasks();
            if (cnt == 1) 
                std::cout << "receive " << cnt << " io task" << std::endl;
            build_sqe();
            // io_uring_submit_and_wait(&m_data_ring, 1);
            // reap_data_tasks();
        };
    }
    int on_complete(context* ctx) {
        if (ctx->m_opcode == 0) {
            // write
            // m_window.commit(context.wal_offset, context.len);
        } else if (ctx->m_opcode == 1) {
            // 直接调用read_task的observer
        }
    };
    void complete_write_task() {
        // 如果write_window的commit index 大于 这个 write_task的cursor，则调用write_task的observer
        // write_window的初始值是recovery设置的
    }
    void complete_read_task() {

    }
    void reap_data_tasks() {    
        struct io_uring_cqe *cqe;
        while(io_uring_peek_cqe(&m_data_ring, &cqe) == 0) {
            auto user_data = static_cast<context*>(io_uring_cqe_get_data(cqe));
            int ret = on_complete(user_data);
            if (ret == 0) {
                // 没出错
                // 则将其携带的buf添加到cache中
                // 对于读请求，首先需要知道什么segment读到了
                // 然后枚举所有读这个segment的请求，取cache
            }
            io_uring_cqe_seen(&m_data_ring, cqe);
        }
        complete_read_task();
        complete_write_task();
    }
    int receive_io_tasks() {
        // TODO: 这里需要限流
        // 从channel里面取出数据，放到pending_task中
        int cnt = 0;
        while (m_channel.size()) {
            m_pending_io_tasks.push_back(m_channel.front());
            m_channel.pop();
            cnt++;
        }
        return cnt;
    }
    void build_read_sqe() {

    }
    void build_write_sqe() {
        // 枚举所有full和current的buf
        // m_buf_writer->write();
        for (auto &item : m_buf_writer->m_full) {            
            auto sqe = io_uring_get_sqe(&m_data_ring); // 从环中得到一块空位
            
            // io_uring_prep_read(sqe, fd, buf, sizeof(buf), 0); // 为这块空位准备好操作

        }
    }
    void build_sqe() {
        // 将pending_task的任务生产sqe
        bool need_write = m_buf_writer->buffering();
        while (m_pending_io_tasks.size()) {
            std::cout << "here" << std::endl;
            auto io_task = m_pending_io_tasks.back();
            m_pending_io_tasks.pop_back();
            if (std::holds_alternative<write_task>(io_task)) {
                // write
                std::cout << "get write task" << std::endl;
                auto task = std::get<write_task>(io_task);
                auto writer = m_buf_writer;
                auto segment = m_wal.segment_file_of(writer->cursor());
                auto payload_length = task.m_data.size();
                if (!segment->can_hold(payload_length)) {
                    segment->append_footer(writer);
                    continue;
                };
                segment->set_written(0);
                auto wal_offset = segment->append_record(writer, task.m_data);
                m_inflight_write_tasks[wal_offset] = task;
                need_write = true;
            } else {
                // read
                auto task = std::get<read_task>(io_task);
                auto segment = m_wal.segment_file_of(task.m_wal_offset);
            }
        }
        build_read_sqe();
        build_write_sqe();
    }
    record_pos add_record(std::string record) {
        // 生产io task
        // 丢给worker线程
        // worker线程攒批，对齐，生产sqe，收割sqe，返回pos
        // m_channel->push(IoTask);
        
    }
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
    std::map<std::string, std::string> m_cached;
    std::thread m_worker;
    std::thread m_compactor;
    write_window m_window;
    // 从channel中获取得到的io任务
    std::vector<std::variant<write_task, read_task>> m_pending_io_tasks;
    // key是cursor，value是对应的任务
    // 当commit index超过cursor时，说明对应的任务已经完成
    // 可以给inflight_write_task的observer发送响应信息了
    std::map<uint64_t, write_task> m_inflight_write_tasks;    
    wal m_wal;
    std::shared_ptr<aligned_buf_writer> m_buf_writer;
    std::vector<io_uring_sqe> m_resubmit_sqes;
    std::queue<std::variant<write_task, read_task>> m_channel;
    std::set<uint64_t> m_barrier;
    std::map<uint64_t, std::pair<context, io_uring_sqe>> m_blocked;
};
 