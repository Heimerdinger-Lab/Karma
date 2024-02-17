#pragma once
#include "options.h"
#include "wal.h"
#include <map>
#include <string>
#include <liburing.h>
#include <thread>
#include <vector>
class record_pos {

};
class configuration {
    int worker_thread_id;
    int compactor_thread_id;
    int uring;
    std::string directory;
};
class io_task {

};
class sivir {
public:
    sivir(open_options& opt) {
        //
        // init the uring
        io_uring_queue_init(32, &m_control_ring, 0);
        io_uring_params params;
        params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
        params.sq_thread_cpu = opt.sqpoll_cpu;
        io_uring_queue_init_params(opt.queue_depth, &m_data_ring, &params);
        // 
        m_worker = std::thread(&sivir::worker_run, this);
        
        // 
        m_compactor = std::thread(&sivir::compactor_run, this);
    };
    ~sivir() {

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
private:
    void init_uring() {}
    void recovery() {
        // 打开目录下所有的文件
        // 遍历一遍，构建内存表

    }
    void worker_run() {
        // loop
        // 对于写io_task
        //  1. 放入alignbuffer
        // 待所有都放完了，按照alignbuffer去生产sqe
        // 对于读io_task
        //  1. 读alignbuffer？

        while(true) {
            if(wal.writable_segment_count() > min_preallocated_segment_files) {
                break;
            }
            wal.try_open_segment();
            // 
            wal.try_close_segment();
            receive_io_tasks();
            build_sqe();
        };
    }
    void receive_io_tasks() {
        // 从channel里面取出数据，放到pending_task中
    }
    void build_read_sqe() {

    }
    void build_write_sqe() {

    }
    void build_sqe() {
        // 将pending_task的任务生产sqe
    }
    record_pos add_record(std::string record) {
        // 生产io task
        // 丢给worker线程
        // worker线程攒批，对齐，生产sqe，收割sqe，返回pos
        
    }
    void compactor_run();
    void check_and_allocated() {
        // check当前预分配的文件
        // 一般要提前分配两个
    }

private:
    io_uring m_control_ring;
    io_uring m_data_ring;
    std::map<std::string, record_pos> m_map;
    std::map<std::string, std::string> m_cached;
    std::thread m_worker;
    std::thread m_compactor;
    std::vector<io_task> m_pending_data_tasks;
    std::vector<io_task> m_inflight_write_tasks;
    wal m_wal;
    AlignedBufWriter buf_writer;
    std::vector<sqe> resubmit_sqes;
};