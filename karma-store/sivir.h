#pragma once
#include "karma-store/buf/aligned_buf.h"
#include "karma-store/buf/aligned_buf_reader.h"
#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-util/sslice.h"
#include "options.h"
#include "wal.h"
#include "write_window.h"
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
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
        // io_uring_queue_init_params(opt.queue_depth, &m_data_ring, &params);
        io_uring_queue_init(opt.queue_depth, &m_data_ring, 0);
        // io_uring_queue_init(opt.queue_depth, &m_data_ring, params.flags);
        // 2. 打开目录下的wal
        m_wal.load_from_path(opt.path);

        // 3. 不断读wal的下一条record，读出record解析，更新memtable
        // record = key + value + wal_offset
        
        std::string str;
        uint64_t wal_offset = 0;
        uint64_t current_offset = 0;
        while(m_wal.scan_record(wal_offset, str)) {
            //
            sslice key, value;
            // decord_record(record, key, value);
            // m_map[key.ToString()] = record_pos {
            //     .wal_offset = current_offset,
            //     .m_value = value,
            // };
            current_offset = wal_offset;
            std::cout << "current_offset = " << current_offset << std::endl;
        };
        // m_wal.try_open_segment();
        // 4. 创建工作线程
        m_buf_writer = std::make_shared<aligned_buf_writer>(wal_offset);
        m_window.commit(0, wal_offset);
        m_window.advance();
        std::cout << "m_window = " << m_window.commit_offset() << std::endl;
        m_worker = std::thread(&sivir::worker_run, this);
        // m_worker.join();
    };
    ~sivir() {
        m_worker.join();
    }
    void recovery() {

    }
    void put(const write_options& opt, std::string key, std::string value) {
        // 调用add_record写wal
        // 得到record_pos
        // 更新memtable和cache
        
        auto pos = add_record("123");

        // update the memtable and the cached
        std::cout << "pos = " << pos.wal_offset << std::endl;
    }
    void del(std::string key) {
        // 
    }
    void get(std::string key, std::string *value, int idx) {
        get_record(11 * idx, 11);
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
        std::shared_ptr<std::promise<write_result>> m_prom;
    };

    struct read_result {
        // [wal_offset, wal_offset + size]
        // sslice m_value;
    };
    struct read_task {
        uint64_t m_wal_offset;
        uint32_t m_len;
        std::string* m_value;
        // observer
        // std::queue<read_result> m_channel;
        std::shared_ptr<std::promise<read_result>> m_prom;
    };
    struct record_pos {
        uint64_t wal_offset;
        uint64_t record_size;
        // [wal_offset + 8, wal_offset + record_size)
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
            if (cnt > 0) 
                std::cout << "receive " << cnt << " io task" << std::endl;
            if (cnt > 0) {
                build_sqe();    
                std::cout << "before" << std::endl;
                io_uring_submit_and_wait(&m_data_ring, 1);
                std::cout << "after" << std::endl;
                reap_data_tasks();
            }
                
        };
    }
    int on_complete(context* ctx) {
        if (ctx->m_opcode == 0) {
            // write
            m_window.commit(ctx->m_wal_offset, ctx->m_buf->limit());
            m_barrier.erase(ctx->m_wal_offset);
        } else if (ctx->m_opcode == 1) {
            // 直接调用read_task的observer
            uint32_t idx = ctx->m_read_idx;
            complete_read_task(idx, ctx);
        }
        return 0;
    };
    void complete_write_task() {
        // 如果write_window的commit index 大于 这个 write_task的cursor，则调用write_task的observer
        // write_window的初始值是recovery设置的
        std::cout << "complete_write_task" << std::endl;
        m_window.advance();
        uint64_t idx = m_window.commit_offset();
        for (auto it = m_inflight_write_tasks.begin(); it != m_inflight_write_tasks.end(); ) {
            if (it->first <= idx) {
                write_result result;
                result.m_wal_offset = it->first - it->second.m_data.size() - 8;
                result.m_size = it->second.m_data.size() + 8;
                it->second.m_prom->set_value(result);
                it = m_inflight_write_tasks.erase(it);       
            } else {
                break;
            }
        }
    }
    void complete_read_task(uint32_t idx, context *ctx) {
        std::cout << "read_task" << std::endl;
        read_result res;
        uint64_t wal_offset = m_inflight_read_tasks[idx].m_wal_offset;
        uint64_t len = m_inflight_read_tasks[idx].m_len;
        m_inflight_read_tasks[idx].m_value->append(ctx->m_buf->buf() + (wal_offset - ctx->m_buf->wal_offset()),  len);
        m_inflight_read_tasks[idx].m_prom->set_value(res);
        // m_inflight_read_tasks[idx].m_channel.push(read_result {});
    }
    void reap_data_tasks() {    
        struct io_uring_cqe *cqe;
        while(io_uring_peek_cqe(&m_data_ring, &cqe) == 0) {
            std::cout << "reap_data_tasks" << std::endl;
            auto user_data = static_cast<context*>(io_uring_cqe_get_data(cqe));
            int ret = on_complete(user_data);
            if (ret == 0) {
                // 没出错
                // 则将其携带的buf添加到cache中
                // 对于读请求，首先需要知道什么segment读到了
                // 然后枚举所有读这个segment的请求，取cache
                std::cout << "end of on_complete" << std::endl;
            }
            io_uring_cqe_seen(&m_data_ring, cqe);
        }
        // complete_read_task();
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
    void build_write_sqe() {
        // 枚举所有full和current的buf
        // m_buf_writer->write();
        for (auto it = m_buf_writer->m_full.begin(); it != m_buf_writer->m_full.end(); ) {   
            auto item = *it;
            if (m_barrier.contains(item->wal_offset())) {
                // 说明还有未完成的 
                continue;
            }
            auto sqe = io_uring_get_sqe(&m_data_ring); // 从环中得到一块空位
            // m_barrier.insert(item->wal_offset());
            auto segment = m_wal.segment_file_of(item->wal_offset());
            io_uring_prep_write(sqe, segment->fd(), item->buf(), item->capacity(), item->wal_offset());
            context *ctx = new context();
            ctx->m_buf = item;
            ctx->m_wal_offset = item->wal_offset();
            ctx->m_len = item->capacity();
            ctx->m_opcode = 0;
            std::cout << "sqe: wal = " << ctx->m_wal_offset << std::endl;
            std::cout << "sqe: len = " << ctx->m_buf->limit() << std::endl;
            std::cout << "sqe: cap = " << ctx->m_buf->capacity() << std::endl;
            io_uring_sqe_set_data(sqe, ctx);
            it = m_buf_writer->m_full.erase(it);
        }
        if (m_buf_writer->m_current->limit() > 0) {
            auto item = m_buf_writer->m_current;
            if (m_barrier.contains(item->wal_offset())) {
                // 说明还有未完成的 
                return;
            }
            auto sqe = io_uring_get_sqe(&m_data_ring); // 从环中得到一块空位
            m_barrier.insert(item->wal_offset());
            auto segment = m_wal.segment_file_of(item->wal_offset());
            io_uring_prep_write(sqe, segment->fd(), item->buf(), item->capacity(), item->wal_offset());
            context *ctx = new context();
            ctx->m_buf = item;
            ctx->m_wal_offset = item->wal_offset();
            ctx->m_len = item->capacity();
            ctx->m_opcode = 0;
            std::cout << "sqe: wal = " << ctx->m_wal_offset << std::endl;
            std::cout << "sqe: len = " << ctx->m_buf->limit() << std::endl;
            std::cout << "sqe: cap = " << ctx->m_buf->capacity() << std::endl;
            io_uring_sqe_set_data(sqe, ctx);
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
                if (!segment->can_hold(payload_length + 8)) {
                    segment->append_footer(writer);
                    segment->set_read_status();
                    continue;
                };
                // segment->set_written(0);
                auto cursor = segment->append_record(writer, task.m_data);
                m_inflight_write_tasks[cursor] = task;
                need_write = true;
            } else {
                // read
                std::cout << "get read task" << std::endl;
                auto task = std::get<read_task>(io_task);
                auto segment = m_wal.segment_file_of(task.m_wal_offset);
                // 
                auto sqe = io_uring_get_sqe(&m_data_ring); // 从环中得到一块空位
                auto buf = aligned_buf_reader::alloc_read_buf(task.m_wal_offset, task.m_len);
                context *ctx = new context();
                ctx->m_buf = buf;
                ctx->m_wal_offset = task.m_wal_offset;
                ctx->m_len = task.m_len;
                ctx->m_opcode = 1;
                ctx->m_read_idx = 2;
                m_inflight_read_tasks[2] = task;
                io_uring_prep_read(sqe, segment->fd(), buf->buf(), buf->capacity(), buf->wal_offset() - segment->wal_offset()); // 为这块空位准备好操作
                io_uring_sqe_set_data(sqe, ctx);
            }
        }
        build_write_sqe();
    }
    record_pos add_record(std::string record) {
        // 生产io task
        // 丢给worker线程
        // worker线程攒批，对齐，生产sqe，收割sqe，返回pos
        // m_channel->push(IoTask);
        std::cout << "record_size = " << record.size() << std::endl;
        write_task task;
        task.m_data = record;
        task.m_prom = std::make_shared<std::promise<sivir::write_result>>();
        m_channel.push(task);

        auto x = task.m_prom->get_future().get();
        std::cout << "x.wal_offset = " << x.m_wal_offset << std::endl;
        std::cout << "x.size = " << x.m_size << std::endl;
        return record_pos{x.m_wal_offset, x.m_size};

        std::string s;
        s.erase(2);
        
    }
    record_pos get_record(uint64_t wal_offset, uint64_t size) {
        // record是从[wal_offset, wal_offset + size)
        // 返回数据是[wal_offset + 8, wal_offset + size)
        read_task task;
        std::string value;
        task.m_value = &value;
        task.m_wal_offset = wal_offset;
        task.m_len = size;
        task.m_prom = std::make_shared<std::promise<sivir::read_result>>();
        m_channel.push(task);
        auto x = task.m_prom->get_future().get();
        
        // auto record = x.m_value;
        // 然后计算crc32

        // 返回 +8位置之后的
        // task.m_value.
        std::cout << "task.value = " << value.erase(0, 8) << std::endl;
        // return record_pos{.m_value = record};
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

    // key是read_task的编号
    std::map<uint32_t, read_task> m_inflight_read_tasks;
    // key是这个buf的wal_offset
    // [key, key + align] 正在inflight
    std::set<uint64_t> m_barrier;
    
    // 这些是阻塞的
    // buf的wal_offset
    // std::map<uint64_t, std::pair<context, io_uring_sqe>> m_blocked;


    wal m_wal;
    std::shared_ptr<aligned_buf_writer> m_buf_writer;
    std::vector<io_uring_sqe> m_resubmit_sqes;
    std::queue<std::variant<write_task, read_task>> m_channel;
    
};
 