#include "mslog.h"

#define TEST_THREAD_CNT 5

void single_thread_test() {
    printf("\n===== 【单线程模式】测试 =====\n");
    char time_buf[32] = {0};
    mslog_utils_get_time_str(time_buf, sizeof(time_buf));
    MSLOG_INFO("TEST", "当前时间：%s | 单线程无锁 + 原生malloc", time_buf);
    MSLOG_WARN("TEST", "日志轮转阈值：%ld Bytes | 工具模块正常生效 ", g_mslog.max_file_size);
}

#ifdef MULTI_THREAD
void* thread_log_task(void *arg) {
    int tid = *(int*)arg;
    for (int i = 1; i <= 3; i++) {
        MSLOG_INFO("MULTI", "线程%d 日志%d写入成功 | 内存池申请：%p", tid, i, mslog_mem_pool_alloc(64));
        mslog_thread_sleep_ms(20);
    }
    return NULL;
}

void multi_thread_test() {
    printf("\n===== 【多线程模式】测试（%d线程）=====\n", TEST_THREAD_CNT);
    pthread_t tid[TEST_THREAD_CNT];
    int tnum[TEST_THREAD_CNT] = {1,2,3,4,5};

    for (int i = 0; i < TEST_THREAD_CNT; i++) {
        mslog_thread_create(&tid[i], MSLOG_THREAD_JOINABLE, thread_log_task, &tnum[i]);
    }
    for (int i = 0; i < TEST_THREAD_CNT; i++) {
        mslog_thread_join(tid[i]);
    }
}
#endif

void print_stat() {
    printf("\n===== 日志统计 & 工具函数验证 =====\n");
    printf("总写入字节数：%ld Bytes\n", g_mslog.total_write_bytes);
    printf("总刷盘次数：%d 次\n", g_mslog.total_flush_time);
    printf("日志文件是否存在：%s\n", mslog_utils_is_file_exist(g_mslog.log_path) ? "是" : "否");
    printf("当前日志文件大小：%d Bytes\n", mslog_utils_get_file_size(g_mslog.log_path));
}

int main() {
    if (mslog_init_default("/tmp/mslog.log", MSLOG_INFO, 1024*1024, 3, MSLOG_FLUSH_BATCH) != 0) {//two mode: 1. MSLOG_FLUSH_REAL_TIME 2. MSLOG_FLUSH_BATCH
        printf("日志库初始化失败！\n");
        return -1;
    }
    printf("日志库初始化成功（集成4大模块：日志+内存池+线程+工具）\n");

    single_thread_test();
#ifdef MULTI_THREAD
    multi_thread_test();
#endif
    print_stat();

    mslog_deinit();
    printf("\n 日志库销毁成功 | 所有测试完成！\n");
    return 0;
}