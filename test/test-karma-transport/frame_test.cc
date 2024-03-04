// #include "karma-transport/frame.h"
// #include "protocol/rpc_generated.h"
// #include <boost/test/tools/old/interface.hpp>
// #include <exception>
// #include <iterator>
// #define BOOST_TEST_MODULE KARMA_TRANSPORT_TEST
// #include <boost/test/unit_test.hpp>
// namespace test {
//     BOOST_AUTO_TEST_SUITE (FrameTest)
//         BOOST_AUTO_TEST_CASE(EncodeTest) {
//             // transport::frame frame0(karma_rpc::OperationCode::OperationCode_HEARTBEAT);
//             // std::string header = "letter from tianpingan";
//             // std::string payload = "hello, motherfucker! Tianpingan is no1";
//             // frame0.set_header(header);
//             // frame0.set_payload(payload);
//             // frame0.flag_response();
//             // auto encoded = frame0.encode();
//             // transport::frame::check(encoded);
//             // auto frame1 = transport::frame::parse(encoded);
//             // std::cout << "frame1_header: " << frame1.m_header << std::endl;
//             // std::cout << "frame1_payload: " << frame1.m_data << std::endl;
//         }
//         BOOST_AUTO_TEST_CASE(CheckTest) {
//             // std::string encoded = "I am a fake encoded string";
//             // try {
//             //     transport::frame::check(encoded);
//             // } catch(transport::frame_error e) {
//             //     std::cout << "e = " << e.is_incomplete() << std::endl;
//             // }

//         }
//         // BOOST_AUTO_TEST_CASE(ConnectionTest) {
//         //     transport::frame frame0(karma_rpc::OperationCode::OperationCode_HEARTBEAT);
//         //     std::string header = "letter from tianpingan";
//         //     std::string payload = "hello, motherfucker! Tianpingan is no1";
//         //     frame0.set_header(header);
//         //     frame0.set_payload(payload);
//         //     frame0.flag_response();
//         //     auto encoded = frame0.encode();
//         //     auto frame1 = transport::frame::parse(encoded);
//         //     std::cout << "frame1_header: " << frame1.m_header << std::endl;
//         //     std::cout << "frame1_payload: " << frame1.m_data << std::endl;
//         // }
//     BOOST_AUTO_TEST_SUITE_END()
// } // namespace test