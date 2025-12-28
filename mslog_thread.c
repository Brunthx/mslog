#define _POSIX_C_SOURCE 200112L
#include "mslog_thread.h"
#include "mslog.h"
#include "mslog_utils.h"
#include <bits/time.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

extern size_t g_total_flush_time;
extern size_t g_flush_count;

static void mslog_thread_do_flush(mslog_global_t *log_ctx);
static void *mslog_thread_worker(void *arg);

static void mslog_thread_do_flush(mslog_global_t *log_ctx){
	if ( log_ctx == NULL || log_ctx->log_fp == NULL || log_ctx->curr_buf_used == 0 || log_ctx->io_buf == NULL ){
		return;
	}

    struct timespec start, end;
    if ( clock_gettime(CLOCK_MONOTONIC, &start) != 0 ){
        return;
    }

    pthread_mutex_lock(&log_ctx->level_mutex[MSLOG_INFO]);

    if ( log_ctx->mmap_buf != MAP_FAILED ){
        if ( log_ctx->mmap_offset + log_ctx->curr_buf_used <= log_ctx->mmap_size ){
            memcpy(( (char *)log_ctx->mmap_buf + log_ctx->mmap_offset ), log_ctx->io_buf, log_ctx->curr_buf_used);
            log_ctx->mmap_offset += log_ctx->curr_buf_used;
            msync(log_ctx->mmap_buf, log_ctx->mmap_offset, MS_ASYNC);
        }
        else {
            fwrite(log_ctx->io_buf, 1, log_ctx->curr_buf_used, log_ctx->log_fp);
        }
    }
    else {
        fwrite(log_ctx->io_buf, 1, log_ctx->curr_buf_used, log_ctx->log_fp);
    }

    fflush(log_ctx->log_fp);
    log_ctx->curr_buf_used = 0;

    pthread_mutex_unlock(&log_ctx->level_mutex[MSLOG_INFO]);

    if ( clock_gettime(CLOCK_MONOTONIC, &end) != 0 ){
        size_t flush_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
        g_total_flush_time += flush_time;
        g_flush_count++;
    }
}

static void *mslog_thread_worker(void *arg){
    mslog_thread_ctx_t *thread_ctx = (mslog_thread_ctx_t *)arg;
    if ( thread_ctx == NULL || thread_ctx->log_ctx == NULL ) {
        return NULL;
    }

    mslog_global_t *log_ctx = thread_ctx->log_ctx;

    while ( thread_ctx->running ) {
        sleep(thread_ctx->flush_interval);

        mslog_thread_do_flush(log_ctx);

        log_ctx->rotate_check_cnt++;
        if ( log_ctx->rotate_check_cnt >= thread_ctx->rotate_check_cnt_max ) {
            mslog_utils_log_rotate(log_ctx);

            log_ctx->rotate_check_cnt = 0;
        }
    }

    mslog_thread_do_flush(log_ctx);

    return NULL;
}

int mslog_thread_init(mslog_thread_ctx_t *thread_ctx, int flush_interval, int rotate_check_cnt_max, mslog_global_t *log_ctx){
    if ( thread_ctx == NULL || log_ctx == NULL || flush_interval <= 0 || rotate_check_cnt_max <= 0) {
        return -1;
    }

    memset(thread_ctx, 0, sizeof(mslog_thread_ctx_t));
    thread_ctx->running = 0;
    thread_ctx->flush_interval = flush_interval;
    thread_ctx->rotate_check_cnt_max = rotate_check_cnt_max;
    thread_ctx->log_ctx = log_ctx;

    thread_ctx->running = 1;
    int ret = pthread_create(&thread_ctx->thread_id, NULL, mslog_thread_worker, thread_ctx);
    if ( ret != 0 ) {
        thread_ctx->running = 0;
        thread_ctx->thread_id = 0;
        return -1;
    }
    return 0;
}

void mslog_thread_destroy(mslog_thread_ctx_t *thread_ctx){
    if ( thread_ctx == NULL) {
        return;
    }

    thread_ctx->running = 0;

    if ( thread_ctx->thread_id != 0 ) {
        pthread_join(thread_ctx->thread_id, NULL);
        thread_ctx->thread_id = 0;
    }

    thread_ctx->flush_interval = 0;
    thread_ctx->rotate_check_cnt_max = 0;
    thread_ctx->log_ctx = NULL;
}
