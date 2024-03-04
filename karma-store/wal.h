#pragma once
#include <algorithm>
#include <boost/noncopyable.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "karma-store/segment_file.h"
#include "karma-util/coding.h"
#include "karma-util/sslice.h"
class wal : public boost::noncopyable {
   public:
    bool load_from_path(std::string directory_path, uint64_t segment_file_size);
    bool scan_record(uint64_t& wal_offset, std::string& str);
    void try_open_segment(uint64_t preallocated_count);
    void try_close_segment(uint64_t first_wal_offset);
    std::optional<std::reference_wrapper<segment_file>> segment_file_of(uint64_t wal_offset);
    void seal_old_segment(uint64_t wal_offset);

   private:
    std::vector<std::unique_ptr<segment_file>> m_segments;
    std::string m_directory_path;
    uint64_t m_segment_file_size = 1024 * 1024;
};
