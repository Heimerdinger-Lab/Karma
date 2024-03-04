#include "wal.h"

#include <optional>

#include "karma-store/common.h"
// 打开所有文件
bool wal::load_from_path(std::string directory_path, uint32_t segment_file_size) {
    m_segment_file_size = segment_file_size;
    m_directory_path = directory_path;
    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (fs::is_regular_file(entry.path())) {
            // entry.path().append("");
            auto filename = entry.path().filename();
            uint64_t wal_offset = std::stoi(filename);
            m_segments.push_back(
                std::make_unique<segment_file>(wal_offset, entry.file_size(), entry.path()));
        }
    }
    std::sort(m_segments.begin(), m_segments.end(),
              [](std::unique_ptr<segment_file>& a, std::unique_ptr<segment_file>& b) -> bool {
                  return a->wal_offset() < b->wal_offset();
              });
    for (const auto& item : m_segments) {
        item->open_and_create(m_segment_file_size);
    }
    return true;
};

// 读offset以后的一条数据buf
/// +---------+-----------+-----------+--- ... ---+
/// |CRC (4B) | Size (3B) | Type (1B) | Payload   |
/// +---------+-----------+-----------+--- ... ---+
bool wal::scan_record(uint64_t& wal_offset, std::string& str) {
    // if (wal_offset)
    for (const auto& item : m_segments) {
        if (item->wal_offset() <= wal_offset && item->wal_offset() + item->size() > wal_offset) {
            int pos = wal_offset - item->wal_offset();
            if (pos + 8 > item->size()) {
                // 说明剩下放不下一个record，就是footer
                // record->clear();
                wal_offset = item->wal_offset() + item->size();
                return true;
            }

            sslice data(std::string(4, ' '));

            // 读4个字节
            item->read_exact_at(&data, wal_offset, 4);

            auto crc32 = DecodeFixed32(data.data());
            std::cout << "crc32 = " << crc32 << std::endl;
            item->read_exact_at(&data, wal_offset + 4, 4);
            auto size_type = DecodeFixed32(data.data());
            auto type = size_type & ((1 << 8) - 1);
            auto size = size_type >> 8;
            std::cout << "size = " << size << std::endl;
            if (pos + store::RECORD_HEADER_LENGTH + size <= item->size()) {
                str.resize(size);
                sslice record(str);
                item->read_exact_at(&record, wal_offset + 8, size);
                //
                if (crc32 == 0) {
                    // 失效
                    item->set_read_write_status();
                    return false;
                }
                wal_offset += store::RECORD_HEADER_LENGTH + size;
                item->set_written(wal_offset - item->wal_offset());
            } else {
            }

            return true;
        }
    }
    return false;
};

void wal::try_open_segment(uint32_t preallocated_count) {
    // 预分配一定的segment file
    // 1. 创建segment_file
    // 2. 调用segment_file.open
    std::vector<std::unique_ptr<segment_file>> segments;
    // if (m_segments.empty()) {
    //     uint64_t wal_offset = 0;
    //     segments.push_back(std::make_shared<segment_file>(wal_offset,
    //     1048576, m_directory_path + "/" + std::to_string(wal_offset)));
    //     wal_offset += 1048576;
    //     segments.push_back(std::make_shared<segment_file>(wal_offset,
    //     1048576, m_directory_path + "/" + std::to_string(wal_offset)));
    // }
    int read_write_cnt = 0;
    for (int i = 0; i < m_segments.size(); i++) {
        if (m_segments[i]->read_write_status()) {
            read_write_cnt++;
        }
    }
    // for (auto& segment : m_segments) {
    //     if (segment->read_write_status()) {
    //         read_write_cnt++;
    //     }
    // }
    // std::cout << "read_write_cnt = " << read_write_cnt << std::endl;
    if (read_write_cnt < preallocated_count) {
        read_write_cnt = preallocated_count - read_write_cnt;
        uint64_t wal_offset = 0;
        if (m_segments.size() > 0) {
            wal_offset = m_segments.back()->wal_offset() + m_segments.back()->size();
        }
        for (int i = 0; i < read_write_cnt; i++) {
            segments.push_back(std::make_unique<segment_file>(
                wal_offset, m_segment_file_size,
                m_directory_path + "/" + std::to_string(wal_offset)));
            wal_offset += m_segment_file_size;
        }
    }
    // if
    for (auto it = segments.begin(); it != segments.end(); it++) {
        auto item = std::move(*it);
        item->open_and_create(m_segment_file_size);
        m_segments.push_back(std::move(item));
    }
}

void wal::try_close_segment(uint64_t first_wal_offset) {}

std::optional<std::reference_wrapper<segment_file>> wal::segment_file_of(uint64_t wal_offset) {
    for (const auto& item : m_segments) {
        if (wal_offset >= item->wal_offset() && wal_offset < (item->wal_offset() + item->size())) {
            return *item;
        }
    }
    return std::nullopt;
}
