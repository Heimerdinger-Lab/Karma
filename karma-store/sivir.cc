#include "sivir.h"

#include <boost/log/trivial.hpp>
#include <cstddef>
#include <exception>

#include "karma-store/common.h"
store::sivir::sivir(){};
// should open it out side the io_context
bool store::sivir::open(open_options &opt) {
    // open
    m_opt = opt;
    io_uring_params params;
    params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
    params.sq_thread_cpu = opt.sqpoll_cpu;
    // int result = io_uring_queue_init_params(opt.queue_depth, &m_data_ring, &params);
    int result = io_uring_queue_init(opt.queue_depth, &m_data_ring, 0);
    if (result < 0) {
        BOOST_LOG_TRIVIAL(error) << "Fail to initialize the io_uring";
        return false;
    }
    result = m_wal.load_from_path(m_opt.path, m_opt.segment_file_size);
    if (!result) {
        BOOST_LOG_TRIVIAL(error) << "Fail to load files from the path: " << opt.path;
        return false;
    }

    // TODO recover
    BOOST_LOG_TRIVIAL(trace) << "Succeed to load files, start to recover from these files";
    std::string record;
    uint64_t start_wal_offset = 0;

    // initialize the writer and window
    m_buf_writer = std::make_unique<aligned_buf_writer>(start_wal_offset);
    m_window.commit(0, start_wal_offset);
    // create an io thread
    m_io_ctx = std::make_unique<co_context::io_context>();
    m_io_ctx->co_spawn(worker_run());
    m_io_ctx->start();
    return true;
}

co_context::task<bool> store::sivir::put(const write_options &opt, std::string &key,
                                         std::string &value) {
    flatbuffers::FlatBufferBuilder cmd_builder;
    auto key_ = cmd_builder.CreateString(key.data());
    auto value_ = cmd_builder.CreateString(value.data());
    auto cmd = karma_rpc::CreateCommand(cmd_builder, karma_rpc::CommandType_VALUE, key_, value_);
    cmd_builder.Finish(cmd);
    auto buffer = cmd_builder.GetBufferPointer();
    int size = cmd_builder.GetSize();
    std::string record_str;
    record_str.append((char *)buffer, size);
    auto pos = co_await add_record(record_str);
    if (!pos.has_value()) {
        co_return false;
    }
    // update the memtable
    m_map[key] = pos.value();
    // m_cached.write(key, value);
    co_return true;
}

co_context::task<bool> store::sivir::del(std::string &key) {
    flatbuffers::FlatBufferBuilder cmd_builder;
    auto key_ = cmd_builder.CreateString(key);
    auto cmd = karma_rpc::CreateCommand(cmd_builder, karma_rpc::CommandType_DELETE, key_);
    cmd_builder.Finish(cmd);
    char *buffer = (char *)cmd_builder.GetBufferPointer();
    int size = cmd_builder.GetSize();
    std::string record_str;
    record_str.append((char *)buffer, size);
    auto pos = co_await add_record(record_str);
    if (!pos.has_value()) {
        co_return false;
    }
    m_map.erase(key);
    // m_cached.erase(key);
    co_return true;
}

co_context::task<bool> store::sivir::get(std::string &key, std::string &value) {
    if (!m_map.contains(key)) {
        co_return false;
    }
    std::string record;
    auto ret = co_await get_record(m_map[key].start_wal_offset, m_map[key].record_size, record);
    if (ret == false) {
        co_return false;
    }
    // skip the record header.
    auto header = flatbuffers::GetRoot<karma_rpc::Command>(record.data() + 8);
    value.append(header->value()->str().data(), header->value()->str().size());
    co_return true;
}

co_context::task<> store::sivir::worker_run() {
    BOOST_LOG_TRIVIAL(trace) << "The worker thread of sivir start to loop...";
    while (true) {
        m_wal.try_open_segment(m_opt.preallocated_count);
        m_wal.try_close_segment(0);
        int cnt = co_await receive_io_tasks();
        if (cnt > 0) {
            build_sqe();
        }
        if (m_inflight_write_tasks.size() > 0 || m_inflight_read_tasks.size() > 0) {
            io_uring_submit(&m_data_ring);
            co_await reap_data_tasks();
        }
    };

    co_return;
}

co_context::task<bool> store::sivir::on_complete(uring_context *ctx) {
    if (ctx->opcode == uring_op::write) {
        auto detail = std::get<write_uring_context>(ctx->detail);
        m_window.commit(detail.buffer->wal_offset(), detail.buffer->limit());
        m_barrier.erase(detail.buffer->wal_offset());
    } else if (ctx->opcode == uring_op::read) {
        auto detail = std::get<read_uring_context>(ctx->detail);
        co_await complete_read_task(detail.read_seq, &detail);
    }
    co_return true;
};

co_context::task<> store::sivir::complete_write_task() {
    uint64_t idx = m_window.current_committed_offset();
    for (auto it = m_inflight_write_tasks.begin(); it != m_inflight_write_tasks.end();) {
        if (it->first <= idx) {
            write_io_result result;
            result.success = true;
            // 这里的常量该怎么取舍比较合适
            result.start_wal_offset = it->first - it->second->data.size() - 8;
            result.size = it->second->data.size() + 8;
            co_context::io_context &current = co_context::this_io_context();
            co_await co_context::resume_on(it->second->m_ctx);
            co_await it->second->m_prom.release(result);
            co_await co_context::resume_on(current);
            it = m_inflight_write_tasks.erase(it);
        } else {
            break;
        }
    }
}

co_context::task<> store::sivir::complete_read_task(uint64_t idx, read_uring_context *ctx) {
    read_io_result res;
    uint64_t wal_offset = m_inflight_read_tasks[idx]->start_wal_offset;
    uint64_t len = m_inflight_read_tasks[idx]->size;
    m_inflight_read_tasks[idx]->data.append(
        ctx->buffer->buf() + (wal_offset - ctx->buffer->wal_offset()), len);

    co_context::io_context &current = co_context::this_io_context();
    co_await co_context::resume_on(m_inflight_read_tasks[idx]->m_ctx);
    co_await m_inflight_read_tasks[idx]->m_prom.release(read_io_result{.success = true});
    co_await co_context::resume_on(current);
    m_inflight_read_tasks.erase(idx);
    co_return;
}

co_context::task<> store::sivir::reap_data_tasks() {
    struct io_uring_cqe *cqe;
    int flag = false;
    while (io_uring_peek_cqe(&m_data_ring, &cqe) == 0) {
        assert(cqe->res >= 0);
        auto user_data = static_cast<uring_context *>(io_uring_cqe_get_data(cqe));
        assert(user_data != NULL);
        co_await on_complete(user_data);
        delete user_data;
        io_uring_cqe_seen(&m_data_ring, cqe);
        flag = true;
    }
    if (flag) {
        co_await complete_write_task();
    }
    co_return;
}

co_context::task<int> store::sivir::receive_io_tasks() {
    // TODO: 这里需要限流
    // 从channel里面取出数据，放到pending_task中
    int cnt = 0;
    m_mtx.lock();
    while (m_channel.size()) {
        auto item = m_channel.front();
        m_channel.pop();
        m_pending_io_tasks.push_back(item);
        cnt++;
    }
    m_mtx.unlock();
    co_return cnt;
}

void store::sivir::build_write_sqe() {
    for (auto it = m_buf_writer->m_full.begin(); it != m_buf_writer->m_full.end();) {
        auto item = *it;
        if (m_barrier.contains(item->wal_offset())) {
            // 说明还有未完成的
            it++;
            continue;
        }
        auto sqe = io_uring_get_sqe(&m_data_ring);  // 从环中得到一块空位
        auto segment_opt = m_wal.segment_file_of(item->wal_offset());
        auto &segment = segment_opt.value().get();
        io_uring_prep_write(sqe, segment.fd(), item->buf(), item->capacity(),
                            item->wal_offset() - segment.wal_offset());
        uring_context *ctx = new uring_context();
        ctx->opcode = uring_op::write;
        ctx->detail = write_uring_context{.buffer = item};
        io_uring_sqe_set_data(sqe, ctx);
        it = m_buf_writer->m_full.erase(it);
    }

    if (m_buf_writer->m_current->limit() > 0 && m_buf_writer->dirty()) {
        auto item = m_buf_writer->m_current;
        if (m_barrier.contains(item->wal_offset())) {
            // 说明还有未完成的
            return;
        }
        auto sqe = io_uring_get_sqe(&m_data_ring);  // 从环中得到一块空位
        m_barrier.insert(item->wal_offset());
        auto segment_opt = m_wal.segment_file_of(item->wal_offset());
        auto &segment = segment_opt.value().get();
        io_uring_prep_write(sqe, segment.fd(), item->buf(), item->capacity(),
                            item->wal_offset() - segment.wal_offset());

        uring_context *ctx = new uring_context();
        ctx->opcode = uring_op::write;
        ctx->detail = write_uring_context{.buffer = item};
        io_uring_sqe_set_data(sqe, ctx);
    }
    m_buf_writer->set_dirty(false);
}

void store::sivir::build_sqe() {
    while (m_pending_io_tasks.size()) {
        auto io_task = m_pending_io_tasks.back();
        m_pending_io_tasks.pop_back();
        if (std::holds_alternative<write_io *>(io_task)) {
            // write
            auto task = std::get<write_io *>(io_task);
            assert(task != NULL);
            auto segment_opt = m_wal.segment_file_of(m_buf_writer->cursor());
            assert(segment_opt.has_value());
            auto &segment = segment_opt.value().get();
            m_wal.seal_old_segment(segment.wal_offset());
            auto payload_length = task->data.size();
            if (!segment.can_hold(payload_length)) {
                segment.append_footer(*m_buf_writer);
                segment.set_read_status();
                assert(task != NULL);
                m_pending_io_tasks.push_back(task);
                continue;
            };
            auto cursor = segment.append_record(*m_buf_writer, task->data);
            m_inflight_write_tasks[cursor] = task;

        } else if (std::holds_alternative<read_io *>(io_task)) {
            // read
            auto task = std::get<read_io *>(io_task);
            auto segment_opt = m_wal.segment_file_of(task->start_wal_offset);
            auto &segment = segment_opt.value().get();
            auto sqe = io_uring_get_sqe(&m_data_ring);  // 从环中得到一块空位
            auto buf = aligned_buf_reader::alloc_read_buf(task->start_wal_offset, task->size);
            io_uring_prep_read(sqe, segment.fd(), buf->buf(), buf->capacity(),
                               buf->wal_offset() - segment.wal_offset());  // 为这块空位准备好操作
            uring_context *ctx = new uring_context();
            ctx->opcode = uring_op::read;
            ctx->detail = read_uring_context{.buffer = buf, .read_seq = ++m_read_seq};
            m_inflight_read_tasks[m_read_seq] = task;
            io_uring_sqe_set_data(sqe, ctx);
        }
    }

    build_write_sqe();
}

co_context::task<std::optional<store::record_pos>> store::sivir::add_record(std::string &record) {
    auto channel = std::make_unique<co_context::channel<write_io_result>>();
    auto task = std::make_unique<write_io>(record, *channel, co_context::this_io_context());
    m_mtx.lock();
    m_channel.push((write_io *)task.get());
    m_mtx.unlock();
    auto result = co_await task->m_prom.acquire();
    if (result.success) {
        co_return record_pos{.start_wal_offset = result.start_wal_offset,
                             .record_size = result.size};
    }
    co_return std::nullopt;
}

co_context::task<bool> store::sivir::get_record(uint64_t wal_offset, uint64_t size,
                                                std::string &value) {
    // record是从[wal_offset, wal_offset + size)
    // 返回数据是[wal_offset + 8, wal_offset + size)
    auto channel = std::make_unique<co_context::channel<read_io_result>>();
    auto task =
        std::make_unique<read_io>(wal_offset, size, value, *channel, co_context::this_io_context());
    m_mtx.lock();
    m_channel.push((read_io *)task.get());
    m_mtx.unlock();
    auto x = co_await task->m_prom.acquire();
    co_return x.success;
}
