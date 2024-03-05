#include "wal.h"

#include <boost/log/trivial.hpp>
#include <cstdint>
#include <optional>
namespace fs = std::filesystem;
#include "karma-store/common.h"
// 打开所有文件
bool wal::load_from_path(std::string directory_path, uint64_t segment_file_size) {
    m_segment_file_size = segment_file_size;
    m_directory_path = directory_path;
    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (fs::is_regular_file(entry.path())) {
            auto filename = entry.path().filename();
            uint64_t wal_offset = std::stoi(filename);
            m_segments.emplace_back(
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
    str.clear();
    for (const auto& item : m_segments) {
        if (item->wal_offset() <= wal_offset && item->wal_offset() + item->size() > wal_offset) {
            item->set_read_write_status();
            uint64_t pos = wal_offset - item->wal_offset();
            if (pos + store::RECORD_HEADER_LENGTH > item->size()) {
                str.resize(item->size() - pos);
                item->set_written(item->size());
                item->set_read_status();
                return true;
            }
            std::string data;
            item->read_exact_at(data, wal_offset, 4);
            str += data;
            auto crc32 = DecodeFixed32(data.data());
            item->read_exact_at(data, wal_offset + 4, 4);
            str += data;
            assert(str.size() == store::RECORD_HEADER_LENGTH);
            auto size_type = DecodeFixed32(data.data());
            auto type = size_type & ((1 << 8) - 1);
            auto size = size_type >> 8;
            if (type == 0) {
                // valid command
                if (pos + store::RECORD_HEADER_LENGTH + size <= item->size()) {
                    item->read_exact_at(data, wal_offset + store::RECORD_HEADER_LENGTH, size);
                    auto crc32_check = crc32c::Value(data.data(), data.size());
                    if (crc32 != crc32_check) {
                        // 失效
                        BOOST_LOG_TRIVIAL(error) << "Corrupt record";
                        return false;
                    }
                    str += data;
                    item->set_written(pos + store::RECORD_HEADER_LENGTH + size);
                    if (item->full()) {
                        item->set_read_status();
                    }
                    return true;
                } else {
                    BOOST_LOG_TRIVIAL(error) << "Corrupt record";
                    return false;
                }
                return true;
            } else if (type == 1) {
                // padding
                str.resize(item->size() - pos);
                item->set_written(item->size());
                item->set_read_status();
                return true;
            }
        }
    }
    return false;
};

void wal::try_open_segment(uint64_t preallocated_count) {
    // 预分配一定的segment file
    // 1. 创建segment_file
    // 2. 调用segment_file.open
    std::vector<std::unique_ptr<segment_file>> segments;

    int read_write_cnt = 0;
    for (int i = 0; i < m_segments.size(); i++) {
        /*
            TODO: Fix Me
            In some case, it will cause segment fault
        */
        assert(m_segments[i].get() != NULL);
        if (m_segments[i]->read_write_status()) {
            read_write_cnt++;
        }
    }

    if (read_write_cnt < preallocated_count) {
        read_write_cnt = preallocated_count - read_write_cnt;
        uint64_t wal_offset = 0;
        if (m_segments.size() > 0) {
            wal_offset = m_segments.back()->wal_offset() + m_segments.back()->size();
        }
        for (int i = 0; i < read_write_cnt; i++) {
            auto temp =
                std::make_unique<segment_file>(wal_offset, m_segment_file_size,
                                               m_directory_path + "/" + std::to_string(wal_offset));
            segments.push_back(std::move(temp));
            wal_offset += m_segment_file_size;
        }
    }

    for (auto it = segments.begin(); it != segments.end(); it++) {
        auto item = (*it)->open_and_create(m_segment_file_size);
        m_segments.push_back(std::move(*it));
    }
}

void wal::try_close_segment(uint64_t first_wal_offset) {}

std::optional<std::reference_wrapper<segment_file>> wal::segment_file_of(uint64_t wal_offset) {
    for (const auto& item : m_segments) {
        if (wal_offset >= item->wal_offset() && wal_offset < (item->wal_offset() + item->size())) {
            return std::ref(*item);
        }
    }
    BOOST_LOG_TRIVIAL(error) << "Fail to find a segment file that contains this wal_offset : "
                             << wal_offset;
    return std::nullopt;
}
void wal::seal_old_segment(uint64_t wal_offset) {
    for (const auto& item : m_segments) {
        if (item->wal_offset() < wal_offset) {
            item->set_read_status();
        }
    }
}
uint64_t wal::get_check_point() {
    // return the first segment file and the first record's index
    if (m_segments.empty()) {
        BOOST_LOG_TRIVIAL(trace) << "Empty directory, the check point is zero";
        return 0;
    }
    // segments are sorted by wal_offset
    return m_segments[0]->wal_offset();
}
