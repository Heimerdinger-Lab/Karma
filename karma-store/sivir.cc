#include "sivir.h"

sivir::sivir(open_options &opt) {
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
    while (m_wal.scan_record(wal_offset, str)) {
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
    m_buf_writer = std::make_unique<aligned_buf_writer>(wal_offset);
    m_window.commit(0, wal_offset);
    m_window.advance();
    std::cout << "m_window = " << m_window.commit_offset() << std::endl;
    // m_worker = std::thread([](std::shared_ptr<sivir> s) -> int{

    //     return 0;
    // }(shared_from_this()));
    // m_worker.join();
};

void sivir::start() {
    // co_context::io_context ctx_worker;
    m_ctx = std::make_unique<co_context::io_context>();
    auto sb = shared_from_this();
    m_ctx->co_spawn([](std::shared_ptr<sivir> s) -> co_context::task<> {
        co_await s->worker_run();
        co_return;
    }(sb));
    m_ctx->start();
    return;
}

co_context::task<> sivir::put(const write_options &opt, std::string key, std::string value) {
    flatbuffers::FlatBufferBuilder cmd_builder;
    auto key_ = cmd_builder.CreateString(key);
    auto value_ = cmd_builder.CreateString(value);
    auto cmd = karma_rpc::CreateCommand(cmd_builder, karma_rpc::CommandType_VALUE, key_, value_);
    cmd_builder.Finish(cmd);
    auto buffer = cmd_builder.GetBufferPointer();
    int size = cmd_builder.GetSize();
    std::cout << "size = " << size << std::endl;
    std::string vv;
    // vv.insert(vv.end(), buffer, size);
    vv.append((char *)buffer, size);
    auto pos = co_await add_record(vv);
    m_map[key] = pos;
    std::cout << "end of put" << std::endl;
    // m_cached.write(key, value);
}

co_context::task<> sivir::del(std::string key) {
    //
    flatbuffers::FlatBufferBuilder cmd_builder;
    auto key_ = cmd_builder.CreateString(key);
    auto cmd = karma_rpc::CreateCommand(cmd_builder, karma_rpc::CommandType_DELETE, key_);
    cmd_builder.Finish(cmd);
    char *buffer = (char *)cmd_builder.GetBufferPointer();
    auto pos = add_record(buffer);
    m_map.erase(key);
    m_cached.erase(key);
    co_return;
}

co_context::task<> sivir::get(std::string key, std::string *value) {
    std::string record;
    co_await get_record(m_map[key].wal_offset, m_map[key].record_size, &record);
    auto header = flatbuffers::GetRoot<karma_rpc::Command>(record.data() + 8);
    value->append(header->value()->str().data(), header->value()->str().size());

    // auto vv = m_cached.read(key);
    // if (vv.has_value()) {
    //     value->append(vv.value().data(), vv.value().size());
    //     co_return;
    // }
    // if (m_map.contains(key)) {
    //     std::string record;
    //     co_await get_record(m_map[key].wal_offset, m_map[key].record_size,
    //     &record); auto header =
    //     flatbuffers::GetRoot<karma_rpc::Command>(record.data() + 8);
    //     value->append(header->value()->str().data(),
    //     header->value()->str().size()); m_cached.write(key, *value);
    // } else {
    //     // sb
    // }
}

co_context::task<> sivir::worker_run() {
    // loop
    // 对于写io_task
    //  1. 放入alignbuffer
    // 待所有都放完了，按照alignbuffer去生产sqe
    // 对于读io_task
    //  1. 读alignbuffer？
    std::cout << "worker_run" << std::endl;
    while (true) {
        // std::cout << "worker_run" << std::endl;
        m_wal.try_open_segment();
        m_wal.try_close_segment(0);
        int cnt = receive_io_tasks();
        if (cnt > 0) {
            build_sqe();
        }
        if (m_inflight_write_tasks.size() > 0 || m_inflight_read_tasks.size() > 0) {
            io_uring_submit(&m_data_ring);
            // io_uring_submit_and_wait(&m_data_ring, 1);
            co_await reap_data_tasks();
        }
    };
    co_return;
}

co_context::task<int> sivir::on_complete(context *ctx) {
    if (ctx->m_opcode == 0) {
        // write
        std::cout << "write: " << ctx->m_wal_offset << ", limit: " << ctx->m_buf->limit()
                  << std::endl;
        m_window.commit(ctx->m_wal_offset, ctx->m_buf->limit());
        m_barrier.erase(ctx->m_wal_offset);
    } else if (ctx->m_opcode == 1) {
        // 直接调用read_task的observer
        uint32_t idx = ctx->m_read_idx;
        co_await complete_read_task(idx, ctx);
    }
    co_return 0;
};

co_context::task<> sivir::complete_write_task() {
    std::cout << "complete_write_task" << std::endl;
    // 如果write_window的commit index 大于 这个
    // write_task的cursor，则调用write_task的observer
    // write_window的初始值是recovery设置的
    // std::cout << "complete_write_task" << std::endl;
    m_window.advance();
    uint64_t idx = m_window.commit_offset();
    std::cout << "commit_offset: " << idx << std::endl;
    for (auto it = m_inflight_write_tasks.begin(); it != m_inflight_write_tasks.end();) {
        if (it->first <= idx) {
            write_result result;
            // assert((it->second.m_data.size()) == 3);
            result.m_wal_offset = it->first - it->second->m_data.size() - 8;
            result.m_size = it->second->m_data.size() + 8;
            std::cout << "release one task" << std::endl;
            it->second->m_ctx.co_spawn([]() -> co_context::task<> {
                std::cout << "hello" << std::endl;
                co_return;
            }());
            co_context::io_context &current = co_context::this_io_context();
            co_await co_context::resume_on(it->second->m_ctx);
            // std::cout << "hello" << std::endl;
            co_await it->second->m_prom.release(result);
            co_await co_context::resume_on(current);
            // it->second->m_ctx.co_spawn([result](std::shared_ptr<co_context::channel<write_result>>
            // prom) -> co_context::task<>{
            //     std::cout << "hello" << std::endl;
            //     co_await prom->release(result);
            //     co_return;
            // }(it->second->m_prom));
            // co_context::condition_variable
            std::cout << "after release: " << it->second->m_prom.size() << std::endl;
            it = m_inflight_write_tasks.erase(it);
        } else {
            break;
        }
    }
}

co_context::task<> sivir::complete_read_task(uint32_t idx, context *ctx) {
    std::cout << "read_task" << std::endl;
    read_result res;
    uint64_t wal_offset = m_inflight_read_tasks[idx]->m_wal_offset;
    uint64_t len = m_inflight_read_tasks[idx]->m_len;
    // assert(len == 11);
    m_inflight_read_tasks[idx]->m_value->append(
        ctx->m_buf->buf() + (wal_offset - ctx->m_buf->wal_offset()), len);
    // assert(m_inflight_read_tasks[idx].m_value->size() == 11);
    if (m_inflight_read_tasks[idx]->m_value->at(10) != '3') {
        std::cout << "cap: " << ctx->m_buf->capacity() << std::endl;
        // assert(m_inflight_read_tasks[idx].m_value->at(10) == '3');
    }
    // co_await m_inflight_read_tasks[idx]->m_prom->release(res);
    co_context::io_context &current = co_context::this_io_context();
    co_await co_context::resume_on(m_inflight_read_tasks[idx]->m_ctx);
    // std::cout << "hello" << std::endl;
    co_await m_inflight_read_tasks[idx]->m_prom.release(read_result{});
    co_await co_context::resume_on(current);
    m_inflight_read_tasks.erase(idx);
    // m_inflight_read_tasks[idx].m_channel.push(read_result {});
    co_return;
}

co_context::task<> sivir::reap_data_tasks() {
    struct io_uring_cqe *cqe;
    int flag = false;
    while (io_uring_peek_cqe(&m_data_ring, &cqe) == 0) {
        // std::cout << "reap_data_tasks" << std::endl;
        // if (cqe->res == )
        std::cout << "cqe->Res: " << cqe->res << std::endl;
        assert(cqe->res >= 0);
        auto user_data = static_cast<context *>(io_uring_cqe_get_data(cqe));
        int ret = co_await on_complete(user_data);
        if (ret == 0) {
            // 没出错
            // 则将其携带的buf添加到cache中
            // 对于读请求，首先需要知道什么segment读到了
            // 然后枚举所有读这个segment的请求，取cache
            std::cout << "end of on_complete" << std::endl;
        }
        free(user_data);
        io_uring_cqe_seen(&m_data_ring, cqe);
        std::cout << "?" << std::endl;
        flag = true;
    }
    // complete_read_task();
    if (flag) {
        co_await complete_write_task();
    }

    co_return;
}

int sivir::receive_io_tasks() {
    // TODO: 这里需要限流
    // 从channel里面取出数据，放到pending_task中
    int cnt = 0;
    while (m_channel.size()) {
        std::cout << "loop" << m_pending_io_tasks.size() << std::endl;
        auto item = m_channel.front();
        m_channel.pop();
        m_pending_io_tasks.push_back(item);
        cnt++;
    }
    return cnt;
}

void sivir::build_write_sqe() {
    // std::cout << "build_write_sqe.cursor: " << m_buf_writer.use_count() <<
    // std::endl; std::cout << "full.size: " << m_buf_writer->m_full->size() <<
    // std::endl; std::cout << "barrier.size: " << m_barrier->size() <<
    // std::endl; 枚举所有full和current的buf m_buf_writer->write();
    if (m_buf_writer->m_full.size()) {
        for (auto it = m_buf_writer->m_full.begin(); it != m_buf_writer->m_full.end();) {
            auto item = *it;
            if (m_barrier.contains(item->wal_offset())) {
                // 说明还有未完成的
                it++;
                continue;
            }
            auto sqe = io_uring_get_sqe(&m_data_ring);  // 从环中得到一块空位
            // m_barrier.insert(item->wal_offset());
            auto segment = m_wal.segment_file_of(item->wal_offset());
            io_uring_prep_write(sqe, segment.fd(), item->buf(), item->capacity(),
                                item->wal_offset() - segment.wal_offset());
            context *ctx = new context();
            ctx->m_buf = item;
            ctx->m_wal_offset = item->wal_offset();
            ctx->m_len = item->capacity();
            ctx->m_opcode = 0;
            std::cout << "1sqe: wal = " << ctx->m_wal_offset << std::endl;
            std::cout << "1sqe: len = " << ctx->m_buf->limit() << std::endl;
            std::cout << "1sqe: cap = " << ctx->m_buf->capacity() << std::endl;
            io_uring_sqe_set_data(sqe, ctx);
            it = m_buf_writer->m_full.erase(it);
        }
    }

    if (m_buf_writer->m_current->limit() > 0 && m_buf_writer->dirty()) {
        auto item = m_buf_writer->m_current;
        if (m_barrier.contains(item->wal_offset())) {
            // 说明还有未完成的
            return;
        }
        auto sqe = io_uring_get_sqe(&m_data_ring);  // 从环中得到一块空位
        m_barrier.insert(item->wal_offset());
        auto segment = m_wal.segment_file_of(item->wal_offset());
        io_uring_prep_write(sqe, segment.fd(), item->buf(), item->capacity(),
                            item->wal_offset() - segment.wal_offset());
        context *ctx = new context();
        ctx->m_buf = item;
        ctx->m_wal_offset = item->wal_offset();
        ctx->m_len = item->capacity();
        ctx->m_opcode = 0;
        std::cout << "2sqe: wal = " << ctx->m_wal_offset << std::endl;
        std::cout << "2sqe: len = " << ctx->m_buf->limit() << std::endl;
        std::cout << "2sqe: cap = " << ctx->m_buf->capacity() << std::endl;
        io_uring_sqe_set_data(sqe, ctx);
    }
    m_buf_writer->set_dirty(false);
}

void sivir::build_sqe() {
    // 将pending_task的任务生产sqe
    // bool need_write = m_buf_writer->buffering();
    while (m_pending_io_tasks.size()) {
        // std::cout << "here" << std::endl;
        auto io_task = m_pending_io_tasks.back();
        m_pending_io_tasks.pop_back();
        if (std::holds_alternative<write_task *>(io_task)) {
            // write
            std::cout << "get write task, cursor" << m_buf_writer->cursor() << std::endl;
            auto task = std::get<write_task *>(io_task);
            // auto writer = m_buf_writer;
            auto segment = m_wal.segment_file_of(m_buf_writer->cursor());
            auto payload_length = task->m_data.size();
            if (!segment.can_hold(payload_length)) {
                std::cout << "can't hold!!!" << std::endl;
                segment.append_footer(*m_buf_writer);
                segment.set_read_status();
                m_pending_io_tasks.push_back(task);
                continue;
            };
            // segment->set_written(0);
            // assert(task.m_data.size() == 11);
            auto cursor = segment.append_record(*m_buf_writer, task->m_data);
            m_inflight_write_tasks[cursor] = task;
            // std::cout << "cursor = " << cursor << std::endl;
            // need_write = true;
        } else if (std::holds_alternative<read_task *>(io_task)) {
            // read
            auto task = std::get<read_task *>(io_task);
            // std::cout << "get read task: " << task->m_wal_offset << std::endl;
            auto segment = m_wal.segment_file_of(task->m_wal_offset);
            //
            auto sqe = io_uring_get_sqe(&m_data_ring);  // 从环中得到一块空位
            auto buf = aligned_buf_reader::alloc_read_buf(task->m_wal_offset, task->m_len);
            std::cout << "task->m_wal_offset: " << task->m_wal_offset << ", len = " << task->m_len
                      << std::endl;
            std::cout << "buf->wal_offset" << buf->wal_offset() << ", size: " << buf->capacity();
            std::cout << "segment = " << segment.fd() << ", wal_offset = " << segment.wal_offset()
                      << std::endl;
            io_uring_prep_read(sqe, segment.fd(), buf->buf(), buf->capacity(),
                               buf->wal_offset() - segment.wal_offset());  // 为这块空位准备好操作

            context *ctx = new context();
            ctx->m_buf = buf;
            ctx->m_wal_offset = task->m_wal_offset;
            ctx->m_len = task->m_len;
            ctx->m_opcode = 1;
            ctx->m_read_idx = 2;
            m_inflight_read_tasks[2] = task;
            io_uring_sqe_set_data(sqe, ctx);
        } else {
            std::cout << "error !!! " << std::endl;
        }
    }
    build_write_sqe();
}

co_context::task<sivir::record_pos> sivir::add_record(std::string record) {
    // 生产io task
    // 丢给worker线程
    // worker线程攒批，对齐，生产sqe，收割sqe，返回pos
    // m_channel->push(IoTask);
    // std::cout << "record_size = " << record.size() << std::endl;
    // write_task task;
    auto channel = std::make_unique<co_context::channel<write_result>>();
    auto task = std::make_unique<write_task>(record, *channel, co_context::this_io_context());
    m_channel.push(task.get());
    std::cout << "before add to record" << std::endl;
    auto x = co_await task->m_prom.acquire();
    std::cout << "x.wal_offset = " << x.m_wal_offset << std::endl;
    std::cout << "x.size = " << x.m_size << std::endl;
    co_return record_pos{x.m_wal_offset, x.m_size};
}

co_context::task<> sivir::get_record(uint64_t wal_offset, uint64_t size, std::string *value) {
    // record是从[wal_offset, wal_offset + size)
    // 返回数据是[wal_offset + 8, wal_offset + size)
    auto channel = std::make_unique<co_context::channel<read_result>>();
    auto task = std::make_unique<read_task>(wal_offset, size, value, *channel,
                                            co_context::this_io_context());
    // co_await m_channel.release(task);
    m_channel.push(task.get());
    auto x = co_await task->m_prom.acquire();
    std::cout << "get_record!!!" << std::endl;
    co_return;
}
