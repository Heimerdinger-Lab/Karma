

// 启动整个服务
void launch() {
    /*
        创建多个线程
        每个线程中创建一个worker，运行worker.serve()
        // woker里面创建三个loop协程
        // n个loop是监听本地fsm的请求
        // 一个loop是监听本地request session manager的请求
        // 一个loop是监听外部request

        // 收到外部的frame后，解析成具体的request
        // 创建一个协程来处理这个request
        // 派发request给具体的fsm实例
        // 得到fsm实例的response后，派发给具体的发送response的实例

    */
}