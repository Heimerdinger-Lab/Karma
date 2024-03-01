#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
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
    void load_from_path(std::string directory_path);
    bool scan_record(uint64_t& wal_offset, std::string& str);
    void try_open_segment();
    void try_close_segment(uint64_t first_wal_offset);
    segment_file& segment_file_of(uint64_t wal_offset);

   private:
    std::vector<std::unique_ptr<segment_file>> m_segments;
    std::string m_directory_path;
};
