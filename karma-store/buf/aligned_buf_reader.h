#pragma once
#include "karma-store/buf/aligned_buf.h"
#include <cstdint>
#include <iostream>
#include <memory>
class aligned_buf_reader {
public:
    static std::shared_ptr<aligned_buf> alloc_read_buf(uint64_t wal_offset, uint64_t len) {
        uint64_t from = wal_offset / aligned_buf_alignment * aligned_buf_alignment;
        uint64_t to = (wal_offset + len + aligned_buf_alignment - 1) / aligned_buf_alignment * aligned_buf_alignment;
        std::cout << "from = " << from << std::endl;
        std::cout << "to = " << to << std::endl;
        return std::make_shared<aligned_buf>(from, (to - from));
    }
};