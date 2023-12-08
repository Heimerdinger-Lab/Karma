#define BOOST_TEST_MODULE KARMA_SERVICE_TEST
#include <boost/test/unit_test.hpp>
namespace test {
    BOOST_AUTO_TEST_SUITE (DemoTest)

        BOOST_AUTO_TEST_CASE(Test01) {
            BOOST_CHECK(true);
        }

        BOOST_AUTO_TEST_CASE(Test02) {
            BOOST_CHECK(true);
        }

    BOOST_AUTO_TEST_SUITE_END()
} // namespace test