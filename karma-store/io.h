#pragma once
#include "buf/aligned_buf_writer.h"
#include <memory>
namespace store {
class io {
public:
    io() {
    };  
    void run() {

    }
private:
    std::shared_ptr<aligned_buf_writer> m_buf_writer;
};
}