// header to std::string (flatbuffer)
// std::string to header (flatbuffer)
// frame是得到std::string的，外部这里需要转换为header


// connection是write frame
// session是write request
// compisiste 是创建request

// client.append_entry(start, target, append_entry_request); 要在所属线程执行clientWrapper， 收到后spawn一个协程去处理它
// client 通过解析target的host和port，得到相关的compisite session，把append_entry_request, start, target都作为参数调用
// compisite session.append_entry()，会将相关参数，构造成request，丢该comisite_session.request()
// comisite_session.request会pick一个session，调用session的wrtie request
// session的write request将会把request序列化成frame，调用connection的write_frame, connection的write为了包装按顺序，就用spawn 一个write task协程一个个弄。
// session也维护着inflight_requests
// session在创建时会有一个read loop哦，收到响应包后会在inflight_reques中查找

// response_session会不断的read request，然后创建任务协程等。


// 作为frame的hander
// #include "karma-raft/common.h"
// #include "karma-raft/rpc_message.h"
// #include "karma-raft/server.h"
#include "scylladb-raft/raft.hh"
#include <span>
namespace client {
// 只是作为header
// class append_entry_request_header {
// public:
//     std::string encode();
//     static append_entry_request_header parse(std::span<char> src);
// public:    
//     raft::server_id m_start;
//     raft::group_id m_group_id;
//     // append request from raft
//     raft::append_request m_request;
// };
// class append_entry_reply_header {
// public:
//     raft::server_id m_start;
//     raft::group_id m_group_id;
//     // 
//     raft::append_reply m_reply;
// };

// class ping_pong_request_header {
// public:
    
// };

// class ping_pong_reply_header {
// public:
// };
// using request_header = std::variant<append_entry_request, append_entry_reply, ping_pong_request, ping_pong_reply>;
}

