#include "write_window.h"

#include <boost/log/trivial.hpp>
void write_window::commit(uint64_t wal_offset, uint64_t len) {
    assert(wal_offset >= 0);
    m_committed[wal_offset] = len;
    advance();
    // BOOST_LOG_TRIVIAL(trace) << "Commit [" << wal_offset << ", " << len
    //                          << ") to write window, now the current committed offset is "
    //                          << m_committed_offset;
};

void write_window::advance() {
    for (auto it = m_committed.begin(); it != m_committed.end();) {
        if (m_committed_offset < it->first) {
            break;
        }
        m_committed_offset = std::max(it->first + it->second, m_committed_offset);
        it = m_committed.erase(it);
    }
}
