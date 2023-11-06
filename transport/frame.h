#pragma once 
#include <iostream>

// 它负责数据的释放
class Frame {


private:
    // 表示是什么操作的frame
    // OperationCode m_code;

    // infligh的seq,用来对应回调
    uint8_t m_seq;
    uint32_t m_raft_group_id;
    const char *m_data;
};