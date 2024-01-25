采用bitcask的结构

内存索引使用radix tree

store这个类在创建时，会创建一个io线程

然后其他线程在共同使用store时，将请求丢给io线程。



## key, value, encoded_record
key, value就是db的key value，是std::string

一条encoded_record是这样的
# +-------------+--------------------+-------------+--------------+---------------+---------+--------------+
# |    type     |  db_id + batch_id  |   key size  |   value size |     expire    |  key    |      value   |
# +-------------+--------------------+-------------+--------------+---------------+--------+--------------+

encoded_record会变成append request {
    length: 
    payload: record是二进制
}
append request完成后，会返回一个index，就是wal中的index，也就是wal中第几个record是当前这个encoded_record的内容。



## record, segment(file), wal
这里面，wal需要使用rocksdb来维持 index（wal中第几个record，以及这个record具体在哪个文件的多少偏移量） 与 (file, offset)的关系



## io, store, db



主要是两层：
- db（对外提供kv的增删改查服务）
    - 类似于bitcask，内存中维护索引（key与wal中的index，该index指明跟该key值最新record的编号）
- wal（对外提供append的服务，返回一个wal中的index）




segment中有很多个大小一致的block

## 我的设计

主要是两层：
- db（对外提供kv的增删改查）
    - 类似于bitcask，内存中维护索引(key, wal_offset, size)
    - 将kv的增删改变成一个record，调用wal的append将record追加进去，并且更新内存中索引
- wal（对外提供append（返回一个wal_offset）和scan(wal_offset, size)服务）
    - 



## AlignedBufWriter
AlignedBuf 就是每次写sqe的buffer大小，默认是4k，也就是内存刷磁盘默认是4k刷

build_seq   
    - build_read_sqe
    - build_write_seq

///
/// +---------+-----------+-----------+--- ... ---+
/// |CRC (4B) | Size (3B) | Type (1B) | Payload   |
/// +---------+-----------+-----------+--- ... ---+
///


## batch的实现

一个batch需要把逻辑record凑在一起，作为AppendRecordRequest丢进去。
这样，它也会AppendRecordRequest变成一个iotask，什么时候它完成了，这个batch就是原子的完成了。

至于indexer的维护，纯在内存中做（包括batch，iotask完成后会返回一个基准wal_offset，batch根据每个requestzhan），已经归档的文件，就把hint写进去新的file中。

batch在用户层去实现



## 总体设计

### wal
wal表示给wal中追加固定数据，返回wal_offset.

一个 append 请求

```
struct AppendRecordRequest {
    int db_id;
    size_t len;
    buffer;
};
```

所以，参数就是buffer就行（这个buffer就是逻辑record encode得来的）

logic record --encode--> buffer ----> physical record

返回一个wal_offset


logic record


key --mem--> logic record index --mem/disk--> wal_offset


----

wal_offset wal.append(data) {

}

所以，读的话，wal是一条条的record的读。


struct physical_offset {
    wal_offset;     // 整个physical record的起点
    physical_len;   //len(data) + header
}


// ---- 
physical_offset wal.append_recode(data) {

}

wal.fetch_record(physical_offset) -> data(data可以parse为logic_record，包含key和value) {

}
// ----


key, physical_offset


然后在恢复的时候，就是重新扫描，建立新的内存表。




## 整体流程：

这里的wal只是代表所有segments

db  -> store -> io

db.put(key, value) {
    physical_offset = co_await store.append_record(data);
    indexer.push(key, physical_offset);
}

db.get(key, value) {
    auto physical_offset = indexer.get(key);
    auto data = co_await store.fetch_record(physical_offset);
    logic_record = parse(data);
    return record_value
}


store.append_record() {
    // 创造io_task
    channel_io.release(io_task);
}

io::loop() {
    get_io_task..
    // io_task.wal_offset，这些需要查询wal，wal就是提供一个查询segments的接口
    build_sqe..
    reap_sqe..
}

一个db有一个store，有一个io


## raft log和状态机的存储
raft log的存储，可以参考raft-engine，使用一个memtable（因为不会有太多，会定时trunck）

但状态机呢？状态机需要把所有内容也放在memory中吗，还是只需要存index。

所以，我觉得状态的存储，只是在memory中存所有的index即可，并且再维护一个block cache来加速。

所以，实现的时候，把wal抽象出来。
至于内存中放什么东西，由上层实现决定，wal恢复时，内存中该怎么重建，也是由上层决定。

所以应该给db设置一个是否启动memtable的设置项。
但用indexer是都需要的。





<!-- Karma -->
    <!-- client -->
    <!-- raft -->
    <!-- raftlog.state_machine -->
    <!-- co_context -->


