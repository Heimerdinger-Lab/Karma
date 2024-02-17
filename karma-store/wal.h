#pragma once
#include "karma-store/segment_file.h"
#include "karma-util/coding.h"
#include "karma-util/sslice.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <liburing.h>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

class wal {
public:
    // 打开所有文件
    void load_from_path(std::string directory_path) {
        m_directory_path = directory_path;
        // std::vector<std::shared_ptr<segment_file>> segments;
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (fs::is_regular_file(entry.path())) {
                // entry.path().append("");
                auto filename = entry.path().filename();
                uint64_t wal_offset = std::stoi(filename);
                m_segments.push_back(std::make_shared<segment_file>(wal_offset, entry.file_size(), entry.path()));
            }
        }   
        std::sort(m_segments.begin(), m_segments.end(), [](std::shared_ptr<segment_file> a, std::shared_ptr<segment_file> b) -> bool {
            return a->wal_offset() < b->wal_offset();
        });
        for (const auto& item: m_segments) {
            item->open_and_create();
        }
    };
    // 读offset以后的一条数据buf
    /// +---------+-----------+-----------+--- ... ---+
    /// |CRC (4B) | Size (3B) | Type (1B) | Payload   |
    /// +---------+-----------+-----------+--- ... ---+
    bool scan_record(uint64_t& wal_offset, sslice* record) {
        // if (wal_offset)
        for (const auto item: m_segments) {
            if (item->wal_offset() <= wal_offset && item->wal_offset() + item->size() > wal_offset) {
                int pos = wal_offset - item->wal_offset();
                if (pos + 8 > item->size()) {
                    // 说明剩下放不下一个record，就是footer
                    record->clear();
                    wal_offset = item->wal_offset() + item->size();
                    return true;
                }
                
                sslice data;

                // 读4个字节
                item->read_exact_at(&data, 4, wal_offset);
                
                auto crc32 = DecodeFixed32(data.data());
                std::cout << "crc32 = " << crc32 << std::endl;
                item->read_exact_at(&data, 4, wal_offset + 4);
                auto size_type = DecodeFixed32(data.data());
                auto type = size_type & ((1 << 8) - 1);
                auto size = size_type & (~((1 << 8) - 1));
                std::cout << "size = " << size << std::endl;
                if (pos + 8 + size < item->size()) {
                    item->read_exact_at(record, size, wal_offset + 8);
                    // 
                    if (crc32 == 0) {
                        // 失效
                        item->set_read_write_status();
                        return false;
                    }
                } else {

                }
                
                return true;                
            }
        }
        return false;
    };
    void try_open_segment() {
        // 预分配一定的segment file
        // 1. 创建segment_file
        // 2. 调用segment_file.open
        std::vector<std::shared_ptr<segment_file>> segments;    
        if (m_segments.empty()) {
            uint64_t wal_offset = 0;
            segments.push_back(std::make_shared<segment_file>(wal_offset, 1048576, m_directory_path + "/" + std::to_string(wal_offset)));
            wal_offset += 1048576;
            segments.push_back(std::make_shared<segment_file>(wal_offset, 1048576, m_directory_path + "/" + std::to_string(wal_offset)));
        }
        for (const auto &item: segments) {
            item->open_and_create();
            m_segments.push_back(item);
        }
    }
    void try_close_segment(uint64_t first_wal_offset) {
        // 如果segment的wal小于first_wal_offset
        // 那么它可以关闭和删除

    }
    std::shared_ptr<segment_file> segment_file_of(uint64_t wal_offset) {
        for (const auto &item: m_segments) {
            if (wal_offset >= item->wal_offset() && wal_offset < item->wal_offset() + item->size()) {
                return item;
            }
        }
        return nullptr;
    }
private:
    std::vector<std::shared_ptr<segment_file>> m_segments;
    std::string m_directory_path;
};

