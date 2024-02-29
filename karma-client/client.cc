// #include "client.h"
// #include "karma-client/header.h"

// // co_context::task<ping_pong_reply>  client::client::ping_pong() {
// //     // ping_pong_request 
// // }

// // run in any thread
// co_context::task<void> client::client::append_entries(raft::server_address start, raft::server_address target, const raft::append_request& append_request) {
//     append_entries_task task {.start = start, .target = target, .append_request = append_request};
//     co_await m_client_task_chan.release(task);
// }
// co_context::task<void> client::client::append_entries_reply(raft::server_address start, raft::server_address target, const raft::append_reply& reply) {
//     append_entries_reply_task task {.start = start, .target = target, .reply = reply};
//     co_await m_client_task_chan.release(task);
// }

// // in the local thread
// co_context::task<void> client::client::local_append_entries(raft::server_address start, raft::server_address target, const raft::append_request& append_request) {
//     auto composite_session = m_session_manager->get_composite_session(target.host, target.id);
//     append_entry_request_header req;
//     req.m_request = append_request;
//     req.m_group_id = 0;
//     req.m_start = start.id;
//     // co_await composite_session->request_to_one(req);
// }

// co_context::task<void> client::client::local_append_entries_reply(raft::server_address start, raft::server_address target, const raft::append_reply& reply) {
//     auto composite_session = m_session_manager->get_composite_session(target.host, target.id);
//     // append_entry_reply req;
//     // req.m_reply = reply;
//     // req.m_group_id = 0;
//     // req.m_start = start.id;
//     // co_await composite_session->request_to_one(req);
// }

// co_context::task<void> client::client::local_ping_pong(ping_pong_task& task) {
//     auto composite_session = m_session_manager->get_composite_session(task.target.host, task.target.id);
//     // ping_pong_request_header req;
//     // 构建出header，payload
//     ping_pong_request_header h{.m_group_id = 0, .m_start = task.start.id, .m_str = "Ping!"};
    
//     // composite_session->request_to_one(h, nullptr, task.m_channel);

// }
