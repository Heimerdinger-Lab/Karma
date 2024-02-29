# Karma

## 项目介绍

## 项目亮点
- 使用io_uring的IO_POLL和SQ_POLL模式的持久化KV模块Sivir
- Sivir支持O_DIRECT绕过PageCache，自己管理缓存，实现了S3FIFO, LRU等缓存算法
- 基于c++ coroutine实现的异步Raft实现，包括异步Apply
- 基于io_uring和c++ coroutine实现RPC，包括one-way rpc，two-way rpc模块。


## Inspired by
- scylladb(raft, cache)
- redpanda(cache)
- seastar(Thread per core, async programming)
- rocksdb(write ahead log)
- co_context(io_uring, coroutine)
- elastic_stream(io_uring, TpC, WAL, session)


## 项目介绍
实验室合作项目，rock-chain是一个，本人主要负责网络模块的开发。

## 工作内容
- 参与基础网络部分的开发
- 实现网络转发功能（组播，单播等），为了满足低通讯设备的通信能力
- 实现Gossip算法，完成成员信息的交换，针对跨域场景进行优化
- 实现路径学习功能，减少网络负担