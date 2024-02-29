#include "protocol/rpc_generated.h"
#include <flatbuffers/buffer.h>
#include <flatbuffers/flatbuffer_builder.h>
#define BOOST_TEST_MODULE 3RD_PART
#include <boost/test/unit_test.hpp>
namespace test {
    BOOST_AUTO_TEST_SUITE (FlatbufferTest) 
        BOOST_AUTO_TEST_CASE(Test01) {
            flatbuffers::FlatBufferBuilder header_builder;
            auto msg = header_builder.CreateString("Hello from the client side!");
            auto sb = karma_rpc::CreatePingPongRequest(header_builder, 666, 0, msg);
            header_builder.Finish(sb);
            uint8_t* buffer = header_builder.GetBufferPointer();
            int size = header_builder.GetSize();
            std::cout << "size = " << size << std::endl;
            std::string encoded;
            encoded.insert(encoded.end(), buffer, buffer + size);
            std::cout << "encoded: " << encoded.size() << std::endl;

            auto s = flatbuffers::GetRoot<karma_rpc::PingPongRequest>(encoded.data());
            std::cout << "msg: " << s->msg()->c_str() << std::endl; 
            std::cout << "id: " << s->from_id() << std::endl;
        };
    BOOST_AUTO_TEST_SUITE_END()
}