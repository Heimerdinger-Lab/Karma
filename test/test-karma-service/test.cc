#include "karma-service/raft/raft_rpc.h"
#define BOOST_TEST_MODULE KARMA_SERVICE_TEST
#include <boost/test/unit_test.hpp>
namespace test {
    BOOST_AUTO_TEST_SUITE (DemoTest)

        BOOST_AUTO_TEST_CASE(raft_rpc_test) {
            // BOOST_CHECK(true);
            raft_rpc rpc;
        }

        BOOST_AUTO_TEST_CASE(Test02) {
            BOOST_CHECK(true);
        }

    BOOST_AUTO_TEST_SUITE_END()
} // namespace test