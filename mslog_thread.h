#pragma once

#ifndef __MSLOG_THREAD_H__
#define __MSLOG_THREAD_H__

#include <pthread.h>
#include <stddef.h>

typedef struct mslog_global mslog_global_t;
typedef struct{
	pthread_t thread_id;
	volatile int running;
	int flush_interval;
	int rotate_check_cnt_max;
	mslog_global_t *log_ctx;
}mslog_thread_ctx_t;

int mslog_thread_init(mslog_thread_ctx_t *thread_ctx, int flush_interval, int rotate_check_cnt_max, mslog_global_t *log_ctx);

void mslog_thread_destroy(mslog_thread_ctx_t *thread_ctx);

#endif//__MSLOG_THREAD_H__
