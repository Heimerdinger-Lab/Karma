#include "sivir.h"

co_context::task<> sivir::reap_data_tasks() {    
    struct io_uring_cqe *cqe;
    int flag = false;
    while(io_uring_peek_cqe(&m_data_ring, &cqe) == 0) {
        // std::cout << "reap_data_tasks" << std::endl;
        // if (cqe->res == )
        std::cout << "cqe->Res: " << cqe->res << std::endl;
        assert(cqe->res >= 0);
        auto user_data = static_cast<context*>(io_uring_cqe_get_data(cqe));
        int ret = co_await on_complete(user_data);
        if (ret == 0) {
            // 没出错
            // 则将其携带的buf添加到cache中
            // 对于读请求，首先需要知道什么segment读到了
            // 然后枚举所有读这个segment的请求，取cache
            std::cout << "end of on_complete" << std::endl;
        }
        free(user_data);
        io_uring_cqe_seen(&m_data_ring, cqe);
        std::cout << "?" << std::endl;
        flag = true;
    }
    // complete_read_task();
    if (flag) {
        co_await complete_write_task();
    }
    
    co_return;
}
