# Karma

## 项目亮点
- 使用io_uring的IO_POLL和SQ_POLL模式的持久化KV模块
- 使用O_DIRECT绕过PageCache，自己管理缓存，实现了S3FIFO, LRU等缓存算法
- 基于c++ coroutine实现的异步Raft实现，包括异步Apply
- 基于io_uring和c++ coroutine实现的网络session模块
- 使用用户态文件系统，将段文件按照冷热安排到SSD上，在用户态定时进行SSD GC



## Inspired by
- scylladb(raft, cache)
- redpanda(cache)
- seastar(Thread per core, async programming)
- rocksdb(write ahead log)
- co_context(io_uring, coroutine)
- elastic_stream(io_uring, TpC, WAL, session)
