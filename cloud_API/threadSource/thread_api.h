// 心跳包线程
void heartBeat_thread(void *arg);
// 网络与服务器的连接线程
void connect_thread(void *arg);
// 服务器下发命令的响应以及设备推送的事件处理线程
void workHandle_thread(void *arg);
// 1 S 定时器线程
void oneSecondTimer_thread(void *arg);
// 异常处理线程
void exceptionHandle_thread(void *arg);
