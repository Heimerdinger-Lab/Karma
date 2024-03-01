#pragma once
#include <sys/types.h>

#include <cstdint>
#include <memory>

#include "protocol/rpc_generated.h"
#include "task.h"

namespace client {
class write_reply : public task {
   public:
    explicit write_reply(bool m_success) : m_success(m_success) {}
    std::unique_ptr<transport::frame> gen_frame() override;
    co_context::task<void> callback(transport::frame &reply_frame) override { co_return; };
    static std::unique_ptr<write_reply> from_frame(transport::frame &frame);
    bool success() { return m_success; }

   private:
    bool m_success;
};
class write_request : public task {
   public:
    write_request(uint64_t m_group_id, std::string m_key, std::string m_value)
        : m_group_id(m_group_id), m_key(std::move(m_key)), m_value(std::move(m_value)) {}
    void set_prom(co_context::channel<std::unique_ptr<write_reply>> *prom) { m_prom = prom; }
    std::unique_ptr<transport::frame> gen_frame() override;
    co_context::task<void> callback(transport::frame &reply_frame) override {
        co_await m_prom->release(write_reply::from_frame(reply_frame));
        std::cout << "callback3" << std::endl;
        co_return;
    };
    static std::unique_ptr<write_request> from_frame(transport::frame &frame);
    std::string key() { return m_key; }
    std::string value() { return m_value; }

   private:
    uint64_t m_group_id;
    std::string m_key;
    std::string m_value;
    co_context::channel<std::unique_ptr<write_reply>> *m_prom;
};
}  // namespace client