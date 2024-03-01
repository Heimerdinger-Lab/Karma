#include "write_window.h"
void write_window::commit(uint64_t wal_offset, uint64_t len) {
    // std::cout << "commit: " << wal_offset << ", len = " << len << std::endl;
    m_committed[wal_offset] = len;
    advance();
};

void write_window::advance() {
    for (auto it = m_committed.begin(); it != m_committed.end();) {
        if (m_commit_offset < it->first) {
            break;
        }
        m_commit_offset = std::max(it->first + it->second, m_commit_offset);
        it = m_committed.erase(it);
    }
    // std::cout << "end of advance" << std::endl;
}
