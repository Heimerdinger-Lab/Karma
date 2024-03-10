#include <boost/test/tools/old/interface.hpp>
#include <exception>
#include <iterator>
#include <memory>
#include <stdexcept>

#include "karma-transport/frame.h"
#include "protocol/rpc_generated.h"
#define BOOST_TEST_MODULE KARMA_TRANSPORT_TEST
#include <boost/test/unit_test.hpp>
namespace test {
BOOST_AUTO_TEST_SUITE(transport_test)
BOOST_AUTO_TEST_CASE(BasicFrameTest) {
    //
    auto frame = std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
    std::string header = "I am header";
    std::string body = "I am body";
    frame->set_header(header);
    frame->set_payload(body);
    frame->flag_response();
    auto encoded_str = frame->encode();
    auto decoded_frame_opt = transport::frame::parse(encoded_str);
    assert(decoded_frame_opt.has_value());
    auto encoded_str2 = decoded_frame_opt.value()->encode();
    assert(encoded_str.compare(encoded_str2) == 0);
    assert(decoded_frame_opt.value()->m_header.compare("I am header") == 0);
    assert(decoded_frame_opt.value()->m_data.compare("I am body") == 0);
}
BOOST_AUTO_TEST_CASE(FrameParseTest) {
    //
    auto frame = std::make_unique<transport::frame>(karma_rpc::OperationCode::OperationCode_ECHO);
    std::string header = "I am header";
    std::string body = "I am body";
    frame->set_header(header);
    frame->set_payload(body);
    frame->flag_response();
    // append some random string after the encoded_str
    auto encoded_str = frame->encode();
    auto encoded_str_tail = encoded_str + "I am an random string";

    auto encoded_str_crc32 = encoded_str;
    encoded_str_crc32.back() = 'F';

    auto encoded_str_frame_size = encoded_str;
    encoded_str_frame_size[0] = 0xFF;
    encoded_str_frame_size[1] = 0xFF;
    encoded_str_frame_size[2] = 0xFF;
    encoded_str_frame_size[3] = 0xFF;
    BOOST_CHECK_NO_THROW(transport::frame::parse(encoded_str_tail));
    BOOST_CHECK_THROW(transport::frame::parse(encoded_str_crc32), std::runtime_error);
    BOOST_CHECK_THROW(transport::frame::parse(encoded_str_frame_size), std::runtime_error);
    auto decoded_frame_opt = transport::frame::parse(encoded_str);
    assert(decoded_frame_opt.has_value());
    auto encoded_str2 = decoded_frame_opt.value()->encode() + "I am an random string";
    assert(encoded_str_tail.compare(encoded_str2) == 0);
    assert(decoded_frame_opt.value()->m_header.compare("I am header") == 0);
    assert(decoded_frame_opt.value()->m_data.compare("I am body") == 0);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace test