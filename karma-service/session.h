#pragma once 
#include "co_context/co/channel.hpp"
#include "co_context/io_context.hpp"
#include "co_context/task.hpp"
#include "karma-service/handler/handler.h"
#include "karma-transport/connection.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#include <memory>
namespace service {
class session {
public:
    session(std::shared_ptr<co_context::socket> socket, std::string addr, uint8_t port) 
    : m_socket(socket)
    , m_inet_address(addr, port) {
    }
    void process() {
        co_context::co_spawn(process0());
    }
    co_context::task<> conn_write(std::shared_ptr<transport::connection> conn, std::shared_ptr<transport::frame> f) {
        co_await conn->write_frame(f);
    };
    co_context::task<> process0() {
        auto conn = std::make_shared<transport::connection>(m_socket, m_inet_address.to_ip(), m_inet_address.port());
        auto channel = std::make_shared<co_context::channel<std::shared_ptr<transport::frame>>>();
        co_context::co_spawn(read_loop(channel, conn));
        co_context::co_spawn(write_loop(channel, conn));
        co_return;
    }
    bool valid() {
        // session是否还存活
        // client和server都需要去检查   
        // connection的析构函数中close socket
        // 而整个链接资源的释放是在session被移除时

        // read和write有一个loop失败，就置该session出错
        // read loop时read 出错，则退出循环
        // write loop时出错，退出循环
        return true;
    }
    co_context::task<void> read_loop(std::shared_ptr<co_context::channel<std::shared_ptr<transport::frame>>> channel, std::shared_ptr<transport::connection> conn) {
        while (true) {
            std::cout << "????" << std::endl;
            std::shared_ptr<transport::frame> f;
            try {
                 f = co_await conn->read_frame();    
            } catch(transport::frame_error e) {
                std::cout <<"frame error" << std::endl;
            }
            printf("f.header = %p, count = %ld\n", f->m_header.data(), f.use_count());
            // std::cout << "!!!!: " << f->m_header.size() <<  std::endl;
            if (f->is_request()) { 
                if (f->m_operation_code == karma_rpc::OperationCode_PING_PONG) {
                    // auto handler = ping_pong_handler(f, channel);
                    std::shared_ptr<ping_pong_handler> handler = std::make_shared<ping_pong_handler>(f, channel);
                    // co_context::co_spawn(handler->call()); 
                    co_await handler->call();
                }
            }
        }
    }
    co_context::task<void> write_loop(std::shared_ptr<co_context::channel<std::shared_ptr<transport::frame>>> channel, std::shared_ptr<transport::connection> conn) {
        // response loop
        while (true) {
            auto f = co_await channel->acquire();
            std::cout << "to write a frame" << std::endl;
            // co_context::co_spawn(conn_write(conn, f));
            co_await conn_write(conn, f);
        }
    }
private:    
    co_context::inet_address m_inet_address;
    std::shared_ptr<co_context::socket> m_socket;
};  
};