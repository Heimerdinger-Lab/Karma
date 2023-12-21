#include "co_context/io_context.hpp"
#include "co_context/net/inet_address.hpp"
#include "co_context/net/socket.hpp"
#include "karma-service/handler/handler.h"
#include "karma-transport/connection.h"
#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#include "co_context/all.hpp"
#include <boost/test/tools/old/interface.hpp>
#include <chrono>
#include <cstring>
#include <iterator>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <thread>
namespace test {

    class temp_session {
    public:
        temp_session(co_context::socket&& socket, std::string addr, uint8_t port) 
        : m_socket(std::move(socket))
        , m_inet_address(addr, port) {
        }
        void process() {
            co_context::co_spawn(process0());
        }
        co_context::task<> process0() {
            transport::connection conn(std::move(m_socket), m_inet_address.to_ip(), m_inet_address.port());
            while (true) {
                auto f = co_await conn.read_frame();    
                std::cout << "??" << f->is_request() <<  std::endl;
                if (f->is_request()) {
                    std::cout << "service receive: " << f->m_operation_code << std::endl;
                    std::cout << "header: " << f->m_header << std::endl;
                    std::cout << "payload: " << f->m_data << std::endl;
                    // 
                    // co_context::co_spawn(service::handler.call())
                }
            }
            // response loop
            while (true) {
                // conn.write(frame f)
            }
        }
    private:    
        co_context::inet_address m_inet_address;
        co_context::socket m_socket;
    };  

    BOOST_AUTO_TEST_SUITE (ConnectionTest)
        co_context::task<void> session(int sockfd) {
            co_context::socket sock{sockfd};
            while (true) {
                char recv_buf[100] = {0};
                int nr = co_await sock.recv(recv_buf);
                std::cout << "recv_buf: " << strlen(recv_buf) << std::endl;
                std::string buf;
                buf.insert(buf.end(), recv_buf, recv_buf + 100);
                // std::string buf = recv_buf;
                std::cout << "receive buf" << buf.size() << std::endl;
                auto f = transport::frame::parse(buf);
                std::cout << "receive frame from tianpingan, header: " << f->m_header << std::endl;
                std::cout << "receive frame from tianpingan, payload: " << f->m_data << std::endl;
            }
        };
        co_context::task<void> server(const uint16_t port) {
            co_context::acceptor ac{co_context::inet_address{port}};
            std::cout << "start to listen at " << port << std::endl;
            for (int sock; (sock = co_await ac.accept()) >= 0;) {
                co_context::socket sb{sock};
                temp_session ts(std::move(sb), std::string("127.0.0.1"), port);
                ts.process();
            }
        }
        co_context::task<void> client() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "in client" << std::endl;
            co_context::inet_address addr;
            auto ret = co_context::inet_address::resolve("127.0.0.1", 1234, addr);
            std::cout << "ret " << ret << std::endl;
            std::cout << "addr" << addr.family() << std::endl;
            co_context::socket sock{co_context::socket::create_tcp(addr.family())};
            int res = co_await sock.connect(addr);
            std::cout << "res = " << res << std::endl;
            auto con = transport::connection(std::move(sock), "127.0.0.1", 1234);
            // transport::frame f(karma_rpc::OperationCode_HEARTBEAT);
            auto f = std::make_shared<transport::frame>(karma_rpc::OperationCode_HEARTBEAT);
            f->set_header("letter from tianpingan");
            f->set_payload("tianpingan is no1");
            co_await con.write_frame(f);
            // transport::frame f2(karma_rpc::OperationCode_HEARTBEAT);
            auto f2 = std::make_shared<transport::frame>(karma_rpc::OperationCode_HEARTBEAT);
            f2->set_header("letter from tianpingan0");
            f2->set_payload("tianpingan is no20");
            co_await con.write_frame(f2);
        }
        BOOST_AUTO_TEST_CASE(ConnectionTest) {
            co_context::io_context ctx1, ctx2;
            ctx1.co_spawn(server(1234));

            ctx2.co_spawn(client());
            ctx1.start();
            ctx2.start();
            ctx1.join();
            ctx2.join();
        } 
    BOOST_AUTO_TEST_SUITE_END()
} // namespace test