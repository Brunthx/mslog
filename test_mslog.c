#include "mslog.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define TEST_ROTATE_SIZE            ( 1024 * 64 )//64kb
#define TEST_THREAD_COUNT           ( 5 )
#define TEST_LOG_COUNT              ( 200 )
#define LOG_ABSOLUTE_PATH           "/tmp/mslog.log"

extern size_t g_total_write_bytes;
extern size_t g_total_flush_time;
extern size_t g_flush_count;
extern mslog_global_t g_mslog;

void *thread_log_write(void *arg){
    int thread_id = *(int *)arg;
    char tag[32] = {0};
    snprintf(tag, sizeof(tag), "THREAD-%02d", thread_id);

    struct timespec ts = {0, 100 * 1000};
    for ( int i = 0; i < TEST_LOG_COUNT; i++ ) {
        MSLOG_DEBUG(tag, "debug log - thread%d, no.%d: mem-pool test", thread_id, i);
        MSLOG_INFO(tag, "info log - thread%d, no.%d: is running right", thread_id, i);
        MSLOG_WARN(tag, "warn log - thread%d, no.%d: warn", thread_id, i);
        MSLOG_ERROR(tag, "error log - thread%d, no.%d: error, should be careful", thread_id, i);
        MSLOG_FATAL(tag, "fatal log - thread%d, no.%d: core thread have problem", thread_id, i);
        nanosleep(&ts, NULL);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    (void)argc;
    (void)argv;
    printf("==============================\n");
    printf("mslog test start!\n");
    printf("==============================\n");

    int ret = mslog_init_default(
            LOG_ABSOLUTE_PATH,
            MSLOG_DEBUG,
            TEST_ROTATE_SIZE,
            3,
            MSLOG_FLUSH_REAL_TIME
            //MSLOG_FLUSH_BATCH
            );

    if ( ret != 0 ) {
        printf("mslog init failed!\n");
        return -1;
    }
    printf("mslog init successed!\n");

    printf("1.single thread multi-level log output test...\n");
    MSLOG_DEBUG("MAIN", "basic debug log for level filter---now is DEBUG level,output");
    MSLOG_INFO("MAIN", "basic info log, green output, file write successed");
    MSLOG_WARN("MAIN", "basic warn log, yellow output, tag is ilegal");
    MSLOG_ERROR("MAIN", "basic error log, red output, core error");
    MSLOG_FATAL("MAIN", "basic fatal log, purple output, core break");
    printf("single thread mslog test finish\n\n");

    printf("2.multi-thread mslog write test(%dthread x %d/thread)...\n", TEST_THREAD_COUNT, TEST_THREAD_COUNT);
    pthread_t threads[TEST_THREAD_COUNT];
    int thread_id[TEST_THREAD_COUNT];
    for ( int i = 0; i < TEST_THREAD_COUNT; i++ ) {
        thread_id[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_log_write, &thread_id[i]);
    }

    for ( int i = 0; i < TEST_THREAD_COUNT; i++ ) {
        pthread_join(threads[i], NULL);
    }
    printf("multi-thread mslog test finish\n\n");

    printf("3.mslog stat count...\n");
    size_t total_bytes, avg_flush_time, buf_usage;
    mslog_get_stats(&total_bytes, &avg_flush_time, &buf_usage);
    printf("total write bytes: %lu Bytes\n", total_bytes);
    printf("total flush time: %lu time\n", g_flush_count);
    printf("avg_flush_time: %lu us\n", avg_flush_time);
    printf("buf_usage: %lu%%\n", buf_usage);
    printf("stat count finish\n\n");

    if ( g_mslog.log_fp != NULL) {
        fflush(g_mslog.log_fp);
        fsync(fileno(g_mslog.log_fp));
    }

    printf("4.mslog deinit test...\n");
    mslog_deinit();
    printf("mslog deinit successed\n");

    printf("\n==============================\n");
    printf("mslog all test passed!\n");
    printf("output log file: mslog.log(main file), mslog.log.1/.2/.3(rotate file)\n");
    printf("===============================\n");

    return 0;
}
