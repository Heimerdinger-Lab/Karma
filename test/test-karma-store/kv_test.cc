#include <fcntl.h>
// #include <liburing.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <memory>

#include "co_context/io_context.hpp"
#include "karma-store/buf/aligned_buf_writer.h"
#include "karma-store/options.h"
#include "karma-store/segment_file.h"
#include "karma-store/sivir.h"
#include "karma-store/write_window.h"
#include "karma-util/coding.h"
// #include "karma-util/sslice.h"
#include "liburing/io_uring.h"
#define BOOST_TEST_MODULE KARMA_STORAGE_TEST
#include <boost/test/unit_test.hpp>
namespace test {
BOOST_AUTO_TEST_SUITE(KVTest)
BOOST_AUTO_TEST_CASE(window_test) {
    write_window window(0);
    uint32_t commit_offset = 0;
    for (int i = 5; i <= 10; i++) {
        window.commit(commit_offset, i);
        commit_offset += i;
    }
    assert(window.current_committed_offset() == (5 + 6 + 7 + 8 + 9 + 10));
}
BOOST_AUTO_TEST_CASE(kv_test) {
    co_context::io_context ctx;
    ctx.co_spawn([]() -> co_context::task<> {
        // store::sivir sv;
        auto sv = std::make_shared<store::sivir>();
        open_options opt;
        opt.queue_depth = 32768;
        opt.sqpoll_cpu = 0;
        opt.path = "/home/tpa/Heimerdinger-Lab/Karma/temp";
        sv->open(opt);
        write_options write_opt;
        std::string key = "Tianpingan";
        std::string value = "no1";
        co_await sv->put(write_opt, key, value);
        std::string ret_value;
        co_await sv->get(key, ret_value);
        assert(ret_value.compare(value));
    }());
    ctx.start();
    ctx.join();
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace test