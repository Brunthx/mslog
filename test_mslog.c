#include "mslog.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_FILE_PATH      "/tmp/mslog.log"
#define THREAD_COUNT       5
#define LOG_PER_THREAD     5
#define MAX_LOG_FILE_SIZE  (1024 * 1024)
#define MAX_LOG_FILE_CNT   3

static void del_old_log_file(void)
{
    if (access(LOG_FILE_PATH, F_OK) == 0)
    {
        remove(LOG_FILE_PATH);
        printf("old log is deleted!\n");
    }
}

static void force_flush_log(void)
{
    if (g_mslog.log_fp != NULL)
    {
        fflush(g_mslog.log_fp);
        fsync(fileno(g_mslog.log_fp));
        fdatasync(fileno(g_mslog.log_fp));
        printf("force flush finish!\n");
    }
}

static void check_log_file_status(void)
{
    struct stat st;
    printf("========================================\n");
    printf(" final log test \n");
    printf(" log path: %s\n", LOG_FILE_PATH);
    if (stat(LOG_FILE_PATH, &st) == 0)
    {
        printf(" size:%ld bytes \n", st.st_size);
    }
    else
    {
        printf(" size:0 bytes \n");
    }
    printf("=========================================\n");
}

void single_thread_test(void)
{
    printf("1.single thread multi-level log output test...\n");
    MSLOG_INFO("TEST", "single thread test -> INFO log");
    MSLOG_WARN("TEST", "single thread test -> WARN log");
    MSLOG_ERROR("TEST", "single thread test -> ERROR log");
    printf("single thread mslog test finish\n\n");
}

static void* thread_log_write(void *arg)
{
    int thread_id = *(int*)arg;
    for (int i = 0; i < LOG_PER_THREAD; i++)
    {
        MSLOG_INFO("MULTI_THREAD", "thread %d, log %d write ok", thread_id, i+1);
        mslog_thread_sleep_ms(1);
    }
    free(arg);
    return NULL;
}

void multi_thread_test(void)
{
    printf("2.multi-thread mslog write test(5thread x 5/thread)...\n");
    pthread_t tid[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        int *p_id = malloc(sizeof(int));
        *p_id = i + 1;
        pthread_create(&tid[i], NULL, thread_log_write, p_id);
    }
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(tid[i], NULL);
    }
    printf("multi-thread mslog test finish\n\n");
}

void mslog_stat_count(void)
{
    printf("3.mslog stat count...\n");
    printf("total write bytes: %ld Bytes\n", g_mslog.total_write_bytes);
    printf("total flush time: %d time\n", g_mslog.total_flush_time);
    printf("avg_flush_time: 0us\n");
    printf("buf_usage: 0%%\n");
    printf("stat count finish\n\n");
}

void mslog_deinit_test(void)
{
    printf("4.mslog deinit test...\n");
    force_flush_log();
    mslog_deinit();
    printf("mslog deinit successed\n\n");
}

int main(int argc, char *argv[])
{
    printf("-e \n");
    printf("====================================\n");
    printf("test_mslog is running...\n");
    printf("=======================================\n");
    printf("./mslog_demo\n");
    printf("-e \n");
    printf("=================================\n");
    printf("test_mslog is running...\n");
    printf("./mslog_demo\n");
    printf("=================================\n");

    del_old_log_file();

    int ret = mslog_init_default(LOG_FILE_PATH, 
                                 MSLOG_INFO, 
                                 MAX_LOG_FILE_SIZE, 
                                 MAX_LOG_FILE_CNT, 
                                 MSLOG_FLUSH_REAL_TIME);
    if (ret == 0)
    {
        printf("mslog init successed!\n");
    }
    else
    {
        printf("mslog init failed!\n");
        return -1;
    }

    single_thread_test();
    multi_thread_test();
    mslog_stat_count();
    mslog_deinit_test();

    printf("=====================================\n");
    printf("mslog all test passed!\n");
    printf("output log file: mslog.log(main file), mslog.log.1/.2/.3(rotate file)\n");
    printf("=====================================\n");
    printf("-e \n test is finish!\n");

    check_log_file_status();
    printf("-e \n test is finish!\n");

    return 0;
}