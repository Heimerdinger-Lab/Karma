#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {

class read_reply : public task {
   public:
    read_reply(bool m_success, std::string m_value)
        : m_success(m_success), m_value(std::move(m_value)) {}
    std::unique_ptr<transport::frame> gen_frame() override;
    co_context::task<void> callback(transport::frame &reply_frame) override { co_return; };
    static std::unique_ptr<read_reply> from_frame(transport::frame &frame);
    std::string value() { return m_value; }

   private:
    bool m_success;
    std::string m_value;
};
class read_request : public task {
   public:
    read_request(uint64_t m_group_id, std::string m_key)
        : m_group_id(m_group_id), m_key(std::move(m_key)) {}
    void set_prom(co_context::channel<std::unique_ptr<read_reply>> *prom) { m_prom = prom; }
    std::unique_ptr<transport::frame> gen_frame() override;
    co_context::task<void> callback(transport::frame &reply_frame) override {
        co_await m_prom->release(read_reply::from_frame(reply_frame));
    };
    static std::unique_ptr<read_request> from_frame(transport::frame &frame);
    std::string key() { return m_key; }

   private:
    uint64_t m_group_id;
    std::string m_key;
    co_context::channel<std::unique_ptr<read_reply>> *m_prom;
};
}  // namespace client