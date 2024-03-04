#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>

#include "karma-store/segment_file.h"
#include "karma-util/coding.h"
#include "karma-util/sslice.h"
// #include <liburing.h>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace fs = std::filesystem;

class wal {
   public:
    bool load_from_path(std::string directory_path, uint32_t segment_file_size);
    bool scan_record(uint64_t& wal_offset, std::string& str);
    void try_open_segment(uint32_t preallocated_count);
    void try_close_segment(uint64_t first_wal_offset);
    std::optional<std::reference_wrapper<segment_file>> segment_file_of(uint64_t wal_offset);

   private:
    std::vector<std::unique_ptr<segment_file>> m_segments;
    std::string m_directory_path;
    uint32_t m_segment_file_size = 1048576;
};
