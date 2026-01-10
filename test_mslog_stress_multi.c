#define _POSIX_C_SOURCE 200112L
#include "mslog_include.h"

// 压测配置（可直接修改）
#define TEST_THREAD_NUM      ( 8 )          // 并发线程数（4/8/16/32，按需调整）
#define TEST_LOG_PER_THREAD  ( 50000 )       // 每个线程打印5万条，总日志=8*5万=40万条
#define TEST_FLUSH_MODE      ( MSLOG_FLUSH_BATCH ) // 切换模式：实时/批量
#define TEST_LOG_FILE        ( "./mslog_stress_multi.log" )

// 全局统计：总耗时
static long long g_total_cost_ms = 0;
// 线程安全锁
static pthread_mutex_t g_cost_mutex = PTHREAD_MUTEX_INITIALIZER;

// 毫秒级时间戳
static long long get_current_time_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// 线程函数：每个线程独立打印海量日志
static void* stress_thread_func(void *arg)
{
    int thread_id = *(int*)arg;
    printf("线程 %d 启动，开始打印 %d 条日志...\n", thread_id, TEST_LOG_PER_THREAD);

    for (int i = 0; i < TEST_LOG_PER_THREAD; i++)
    {
        // 日志带线程ID，方便后续校验「是否乱序/交叉」
        MSLOG_INFO(STRESS_TEST_TAG,
                   "stress test multi thread | tid=%d | idx=%d | rand=%d | msg=high concurrency log test",
                   thread_id, i, rand() % 100000);
    }

    printf("线程 %d 完成，已打印 %d 条日志\n", thread_id, TEST_LOG_PER_THREAD);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    printf("===== mslog 多线程压力测试开始 =====\n");
    printf("压测配置：线程数=%d | 单线程日志=%d | 总日志=%d | 刷盘模式=%s\n",
           TEST_THREAD_NUM, TEST_LOG_PER_THREAD,
           TEST_THREAD_NUM * TEST_LOG_PER_THREAD,
           TEST_FLUSH_MODE == MSLOG_FLUSH_REAL_TIME ? "实时刷盘" : "批量刷盘");
    printf("日志文件：%s\n\n", TEST_LOG_FILE);

    // 1. 初始化日志库（扩容日志文件，避免压测中轮转）
    if (mslog_init_default(TEST_LOG_FILE,
                           MSLOG_INFO,
                           1024*1024*100, // 100MB日志文件
                           5,
                           TEST_FLUSH_MODE) != 0)
    {
        fprintf(stderr, "日志库初始化失败！\n");
        return -1;
    }

    // 2. 创建多线程，启动压测
    pthread_t tids[TEST_THREAD_NUM];
    int thread_ids[TEST_THREAD_NUM];
    long long start_ms = get_current_time_ms();

    for (int i = 0; i < TEST_THREAD_NUM; i++)
    {
        thread_ids[i] = i;
        if (pthread_create(&tids[i], NULL, stress_thread_func, &thread_ids[i]) != 0)
        {
            fprintf(stderr, "创建线程 %d 失败！\n", i);
            return -1;
        }
    }

    // 3. 等待所有线程执行完毕
    for (int i = 0; i < TEST_THREAD_NUM; i++)
    {
        pthread_join(tids[i], NULL);
    }
    long long end_ms = get_current_time_ms();
    g_total_cost_ms = end_ms - start_ms;

    // 4. 销毁日志库，兜底刷盘
    mslog_deinit();
    pthread_mutex_destroy(&g_cost_mutex);

    // 5. 多线程压测指标统计（核心）
    int total_log_count = TEST_THREAD_NUM * TEST_LOG_PER_THREAD;
    double tps = (double)total_log_count / (g_total_cost_ms / 1000.0);
    printf("\n===== 多线程压测结果统计 =====\n");
    printf("✅ 总日志条数：%d 条\n", total_log_count);
    printf("✅ 并发线程数：%d 个\n", TEST_THREAD_NUM);
    printf("✅ 总耗时     ：%lld ms（%.2f s）\n", g_total_cost_ms, g_total_cost_ms/1000.0);
    printf("✅ 吞吐量(TPS)：%.2f 条/秒\n", tps);
    printf("✅ 总写入字节 ：%zu Bytes（%.2f MB）\n", g_mslog.total_write_bytes, (double)g_mslog.total_write_bytes/1024/1024);
    printf("✅ 刷盘总次数 ：%d 次\n", g_mslog.total_flush_time);
    printf("================================\n");

    return 0;
}