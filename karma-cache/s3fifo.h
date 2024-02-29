#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "karma-cache/ring_buffer.h"
#pragma
namespace cache {
class s3fifo {
   public:
    std::optional<std::string> read(std::string key) {
        auto it = key_to_loc_.find(key);
        if (it == key_to_loc_.end()) {
            return std::nullopt;
        } else {
            return data_[it->second];
        }
    };
    void write(std::string key, std::string value) {
        auto it = key_to_loc_.find(key);
        uint32_t loc = 0;

        if (it == key_to_loc_.end()) {
            // 三个队列都不在，则分配一个，然后插入到small中
            insert_to_small(key, value);
            loc = key_to_loc_.at(key);
        } else if (ghost_contents_.contains(it->second)) {
            // 在ghost队列，则插入到main
            insert_to_main(key, value);
            loc = it->second;
        };
        if (freq_[loc] < 3) {
            ++freq_[loc];
        }
        data_[loc] = value;
    }
    void erase(std::string key) {}

   private:
    void insert_to_small(std::string key, std::string value) {
        while (small_.IsFull()) {
            evict_small();
        }
        uint32_t loc = free_loc_.Pop();
        small_.Push(loc);
    }
    void insert_to_main(std::string key, std::string value) {
        while (main_.IsFull()) {
            evict_main();
        }
        uint32_t loc = free_loc_.Pop();
        main_.Push(loc);
    }
    void insert_to_ghost(std::string key, std::string value) {
        while (ghost_.IsFull()) {
            evict_main();
        }
        uint32_t loc = free_loc_.Pop();
        ghost_.Push(loc);
    }
    void evict_small() {
        // 将small中移动到ghost
        uint32_t eviction_candidate = small_.Pop();
        if (freq_[eviction_candidate] <= 1) {
            // insert_to_ghost();
            return;
        }
        if (main_.IsFull()) {
            evict_main();
        }
        main_.Push(eviction_candidate);
    }
    void evict_main() {
        while (main_.Size() > 0) {
            uint32_t eviction_candidate = main_.Pop();
            if (freq_[eviction_candidate] == 0) {
                break;
            }
            freq_[eviction_candidate] -= 1;
            main_.Push(eviction_candidate);
        }
    }
    void clean(uint32_t loc) {
        free_loc_.Push(loc);
        key_to_loc_.erase(loc_to_it_[loc]);
    }
    // 3个fifo
    RingBuffer<uint32_t, 10> small_;
    RingBuffer<uint32_t, 10> main_;
    RingBuffer<uint32_t, 10> ghost_;
    // 一个free location数组
    RingBuffer<uint32_t, 20> free_loc_; /* could be reduced in size */
                                        // 一个key to location映射
    std::unordered_map<std::string, uint32_t> key_to_loc_;

    std::unordered_set<uint32_t> ghost_contents_;
    // 每个location的频率
    uint8_t freq_[30]; /* could be reduced in size */
    // uint8_t valid_[30];
    std::unordered_map<uint32_t, std::unordered_map<std::string, uint32_t>::iterator> loc_to_it_;
    // local to value
    std::string data_[30];

    // key -> location -> value
    // location 会在 small, main, ghost 之间转换
};
};  // namespace cache