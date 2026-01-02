#define _POSIX_C_SOURCE 200112L
#include "mslog.h"

#define TEST_LOG_TOTAL         ( 100000 )//100000 pass!1000000 pass!
#define TEST_FLUSH_MODE        ( MSLOG_FLUSH_BATCH )//MSLOG_FLUSH_BATCH/MSLOG_FLUSH_REAL_TIME

static long long get_current_time_ms(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ( ts.tv_sec * 1000 + ts.tv_nsec / 1000000 );
}

int main(int argc, char *argv[])
{
    printf("===== mslog 单线程压力测试开始 =====\n");
    printf("压测配置：日志总数=%d 刷盘模式=%s\n", 
           TEST_LOG_TOTAL, 
           TEST_FLUSH_MODE == MSLOG_FLUSH_REAL_TIME ? "实时刷盘" : "批量刷盘");
    printf("日志文件：./mslog_stress_single.log\n\n");

    // 1. 初始化日志库（复用现有接口，切换刷盘模式仅改此行）
    if (mslog_init_default("./mslog_stress_single.log", 
                           MSLOG_INFO, 
                           1024*1024*50,  // 日志文件扩容到50MB，避免压测中频繁轮转
                           5, 
                           TEST_FLUSH_MODE) != 0)
    {
        fprintf(stderr, "日志库初始化失败！\n");
        return -1;
    }

    // 2. 压测核心：循环打印海量日志，记录耗时
    long long start_ms = get_current_time_ms();
    for (int i = 0; i < TEST_LOG_TOTAL; i++)
    {
        // 模拟业务日志：包含数字、字符串、浮点型，贴近真实场景
        MSLOG_INFO(STRESS_TEST_TAG, 
                   "stress test single thread | idx=%d | rand_val=%d | float_val=%.2f | msg=mslog pressure test success",
                   i, rand() % 10000, (float)i/100.0);
        
        // 每1万条打印进度，避免终端无输出
        if (i % 10000 == 0 && i > 0)
        {
            printf("进度：已打印 %d 条日志\n", i);
        }
    }
    long long end_ms = get_current_time_ms();

    // 3. 销毁日志库（批量模式下：触发兜底刷盘，确保日志100%落盘）
    mslog_deinit();

    // 4. 性能指标统计 & 输出（核心！量化压测结果）
    long long total_cost_ms = end_ms - start_ms;
    double tps = (double)TEST_LOG_TOTAL / (total_cost_ms / 1000.0); // 每秒日志数
    printf("\n===== 单线程压测结果统计 =====\n");
    printf("✅ 总日志条数：%d 条\n", TEST_LOG_TOTAL);
    printf("✅ 总耗时     ：%lld ms（%.2f s）\n", total_cost_ms, total_cost_ms/1000.0);
    printf("✅ 吞吐量(TPS)：%.2f 条/秒\n", tps);
    printf("✅ 总写入字节 ：%zu Bytes（%.2f MB）\n", g_mslog.total_write_bytes, (double)g_mslog.total_write_bytes/1024/1024);
    printf("✅ 刷盘总次数 ：%d 次\n", g_mslog.total_flush_time);
    printf("================================\n");

    return 0;
}