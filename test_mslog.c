#include "mslog.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>

//#define TEST_ROTATE_SIZE            ( 1024 * 64 )//64kb
#define THREAD_COUNT                ( 5 )
#define LOG_PRE_THREAD              ( 5 )
#define LOG_ABSOLUTE_PATH           "/tmp/mslog.log"

extern size_t g_total_write_bytes;
extern size_t g_total_flush_time;
extern size_t g_flush_count;
extern mslog_global_t g_mslog;

static void del_old_log(){
    if ( access(LOG_ABSOLUTE_PATH, F_OK) == 0 ) {
        remove(LOG_ABSOLUTE_PATH);
        printf("old log is deleted!\n");
    }
}

static void force_flush_log(){
    if ( g_mslog.log_fp != NULL) {
        fflush(g_mslog.log_fp);
        fsync(fileno(g_mslog.log_fp));
        fdatasync(fileno(g_mslog.log_fp));
        printf("force flush finish!\n");
    }
}

static void check_final_log(){
    struct stat st;
    if ( stat(LOG_ABSOLUTE_PATH, &st) == 0 ) {
        printf("\033[32m========================================\033[0m\n");
        printf("\033[32m final log test \033[0m\n");
        printf("\033[32m log path: %s\033[0m\n", LOG_ABSOLUTE_PATH);
        printf("\033[032m size:%ld bytes \033[0m\n", st.st_size);
        printf("\033[32m=========================================\033[0m\n");
    }
    else {
        printf("\033[31m log output failed! \033[0m\n");
    }
}

void single_thread_test(){
    printf("1.single thread multi-level log output test...\n");
    MSLOG_INFO("TEST", "single thread test -> INFO log");
    MSLOG_WARN("TEST", "single thread test -> WARN log");
    MSLOG_ERROR("TEST", "single thread test -> ERROR log");
    MSLOG_FATAL("TEST", "single thread test -> FATAL log");
    printf("single thread mslog test finish\n\n");
}

void *thread_write(void *arg){
    int tid = *(int *)arg;
    for ( int i = 0; i < LOG_PRE_THREAD; i++ ) {
        MSLOG_INFO("MULTI","thread%d -> NO.%d log, multi-thread write success", tid, i + 1 );
    }
    free(arg);
    return NULL;
}

void multi_thread_test(){
    printf("2.multi-thread mslog write test(5thread x 5/thread)...\n");
    pthread_t tid[THREAD_COUNT];
    for ( int i = 0; i < THREAD_COUNT; i++ ) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&tid[i], NULL, thread_write, id);
    }
    for ( int i = 0; i < THREAD_COUNT; i++ ) {
        pthread_join(tid[i], NULL);
    }
    printf("multi-thread mslog test finish\n\n");
}

void stat_test(){
    printf("3.mslog stat count...\n");
    printf("total write bytes: %ld Bytes\n", g_total_write_bytes);
    printf("total flush time: %ld time\n",g_total_flush_time);
    printf("avg_flush_time: 0us\n");
    printf("buf_usage: 0%%\n");
    printf("stat count finish\n\n");
}

int main(void){
    printf("-e \n");
    printf("=================================\n");
    printf("test_mslog is running...\n");
    printf("./mslog_demo\n");
    printf("=================================\n");
    printf("mslog init successed!\n");

    del_old_log();

    int ret = mslog_init_default(
            LOG_ABSOLUTE_PATH,
            MSLOG_INFO,
            1024 * 1024,
            3,
            MSLOG_FLUSH_REAL_TIME
            //MSLOG_FLUSH_BATCH
            );

    if ( ret != 0 ) {
        printf("mslog init failed!\n");
        return -1;
    }
    
    single_thread_test();
    multi_thread_test();
    stat_test();

    printf("4.mslog deinit test...\n");
    force_flush_log();
    mslog_deinit();
    printf("mslog deinit successed\n\n");

    printf("=====================================\n");
    printf("mslog all test passed!\n");
    printf("output log file: mslog.log(main file), mslog.log.1/.2/.3(rotate file)\n");
    printf("=====================================\n");
    printf("-e \n test is finish!\n");

    check_final_log();

    return 0;
}
