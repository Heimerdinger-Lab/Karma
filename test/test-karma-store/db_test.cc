#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-store/options.h"
#include "karma-store/segment_file.h"
#include "karma-store/sivir.h"
#include "karma-util/coding.h"
#include "karma-util/sslice.h"
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <liburing.h>
#include <memory>
#include <unistd.h>
#define BOOST_TEST_MODULE KARMA_STORAGE_TEST
#include <boost/test/unit_test.hpp>
namespace test {
    BOOST_AUTO_TEST_SUITE (DBTest)

        BOOST_AUTO_TEST_CASE(sivir_test) {
            open_options config;
            config.queue_depth = 32768;
            config.sqpoll_cpu = 0;
            config.path = "/home/tpa/Heimerdinger-Lab/Karma/temp";
            sivir db(config);
            db.put(write_options{}, "0123456789", "abcde");
            // std::string value;
            // db.get("0123456789", &value);
            // db.del("0123456789");
            // assert(value == "value");
        }
        BOOST_AUTO_TEST_CASE(wal_test) {
            // BOOST_CHECK(true);
            // wal temp_wal;
            // temp_wal.load_from_path("/home/tpa/Heimerdinger-Lab/Karma/temp");
            // temp_wal.try_open_segment();
            // temp_wal.scan_record(, );
            // 给segment
        }
        
        BOOST_AUTO_TEST_CASE(aligned_buf_writer_test) {
            // segment file
            // aligned_buf_writer writer;
            
        }
//         struct context {
//             // sqe 的 user_data
//             uint8_t m_opcode;
//             // 写和读的数据
//             std::shared_ptr<aligned_buf> m_buf;
//             // 写的情况下，m_wal_offset是m_buf的wal_offset
//             // 读的情况下，就是要读的位置
//             uint64_t m_wal_offset;
//             uint64_t m_len;

//             // 对于写
//             //  commit(m_buf.wal_offset, m_buf.wal_offset + aligned_buf.limit());

//             // 对于读
//             // 拷贝[m_wal_offset, m_len)到read_result
//         };
//         BOOST_AUTO_TEST_CASE(segment_test) {
//             segment_file file(0, 1048576, "/home/tpa/Heimerdinger-Lab/Karma/temp2/temp");
//             file.open_and_create();
//             auto writer = std::make_shared<aligned_buf_writer>(0);
//             sslice data1("0123456789");
//             file.append_record(writer, data1);
//             // sslice data2("Fuck from tianpingan");
//             // file.append_record(writer, data2);
//             // file.append_footer(writer);
//             io_uring ring;
//             int x = io_uring_queue_init(32, &ring, 0); // 初始化
//             if (x != 0) {
//                 return;
//             }
//             // // 生成sqe
//             auto full = writer->m_full;
//             auto current = writer->m_current;
//             int cnt = 0;
//             for (auto &item : full) {
//                 auto sqe = io_uring_get_sqe(&ring);
//                 io_uring_prep_write(sqe, 0, item->buf(), item->capacity(), item->wal_offset());
//                 context* ctx = (context *)malloc(sizeof( context));
//                 ctx->m_buf = item;
//                 io_uring_sqe_set_data(sqe, ctx);
//                 cnt++;
//             }
//             if (current->limit() > 0) {
//                 auto sqe = io_uring_get_sqe(&ring);
//                 for (int i = 0; i < current->limit(); i++) {
//                     printf("buf[%d] = %x\n", i, current->buf()[i]);
//                 }
//                 std::cout << "fd = " << file.fd() << std::endl;
//                 // int ret = ::write(file.fd(), current->buf(), current->capacity());
                
//                 // int ret = ::write(file.fd(), "123", 3);

//                 // std::cout << errno << std::endl;
//                 // std::cout << "ret = " << ret << std::endl;
// // /                std::cout << errno << std::endl;
//                 // ::fsync(file.fd());

//                 io_uring_prep_write(sqe, file.fd(), current->buf(), current->capacity(), current->wal_offset());
//                 context *ctx = new context();
//                 ctx->m_buf = current;
//                 ctx->m_wal_offset = current->wal_offset();
//                 ctx->m_len = current->capacity();
//                 ctx->m_opcode = 0;
//                 std::cout << "sqe: wal = " << ctx->m_wal_offset << std::endl;
//                 std::cout << "sqe: len = " << ctx->m_buf->limit() << std::endl;
//                 std::cout << "sqe: cap = " << ctx->m_buf->capacity() << std::endl;
//                 io_uring_sqe_set_data(sqe, ctx);
//                 cnt++;
//             }
//             // // 执行完
//             std::cout << "cnt = " << cnt << std::endl;
//             io_uring_submit_and_wait(&ring, cnt); // 提交任务
//             io_uring_cqe* res; // 完成队列指针
//             io_uring_wait_cqe(&ring, &res); // 阻塞等待一项完成的任务
//             context * ret = (context *)res->user_data;
//             std::cout << "ret: wal = " << ret->m_wal_offset << std::endl;
//             std::cout << "ret: len = " << ret->m_buf->limit() << std::endl;
            
//             // // 然后再读
//             std::string str1(4, ' ');
//             sslice s1(str1);
//             file.read_exact_at(&s1, s1.size(), 0);
//             uint32_t crc32 = DecodeFixed32(str1.data());            
//             std::cout << "crc32 = " << crc32 << std::endl; 

//             std::string str2(4, ' ');
//             sslice s2(str2);
//             file.read_exact_at(&s2, s2.size(), 4);
//             uint32_t length_type = DecodeFixed32(str2.data());
//             // std::cout << "str2[0]: " << str2.data()[0] << ", " << str2.data()[1] << str2.data()[2] << str2.data()[3] << std::endl; 
//             std::cout << "length_type = " << length_type << std::endl;
//             std::cout << "length = " << (length_type >> 8) << std::endl;
//             std::cout << "type = " << (length_type & ((1 << 8) - 1)) << std::endl;
//             std::string s3(length_type >> 8, ' ');
//             sslice record(s3);
//             file.read_exact_at(&record, record.size(), 8);
//             std::cout << "record = " << s3 << std::endl;
//         }
        // BOOST_AUTO_TEST_CASE(build_sqe_test) {
        //     // BOOST_CHECK(true);
        //     open_options opt;
        //     opt.path = "/home/tpa/Heimerdinger-Lab/Karma/temp";
        //     opt.queue_depth = 128;
        //     opt.sqpoll_cpu = 0;
        //     sivir sv(opt);
        //     sivir::write_task task;
        //     task.m_data = "abcd";
        //     task.m_prom = std::make_shared<std::promise<sivir::write_result>>();
            
        //     sv.m_channel.push(task);

        //     auto result = task.m_prom->get_future().get();
        //     std::cout << "wal_offset = " << result.m_wal_offset << ", size = " << result.m_size << std::endl; 
        // }
        // BOOST_AUTO_TEST_CASE(build_sqe_test) {
        //     // BOOST_CHECK(true);
        //     open_options opt;
        //     opt.path = "/home/tpa/Heimerdinger-Lab/Karma/temp";
        //     opt.queue_depth = 128;
        //     opt.sqpoll_cpu = 0;
        //     sivir sv(opt);
        //     sivir::write_task task;
        //     task.m_data = "abcd";
        //     task.m_prom = std::make_shared<std::promise<sivir::write_result>>();
            
        //     sv.m_channel.push(task);

        //     auto result = task.m_prom->get_future().get();
        //     std::cout << "wal_offset = " << result.m_wal_offset << ", size = " << result.m_size << std::endl; 
        // }
        

    BOOST_AUTO_TEST_SUITE_END()
} // namespace test