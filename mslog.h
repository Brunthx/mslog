#ifndef __MSLOG_H__
#define __MSLOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include "mslog_mem_pool.h"
#include "mslog_utils.h"
#include "mslog_thread.h"

//color setting
#define MSLOG_COLOR_DEBUG   "\033[030m"//grey
#define MSLOG_COLOR_INFO    "\033[032m"//green
#define MSLOG_COLOR_WARN    "\033[033m"//yellow
#define MSLOG_COLOR_ERROR   "\033[031m"//red
#define MSLOG_COLOR_FATAL   "\033[035m"//purple
#define MSLOG_COLOR_RESET   "\033[0m"//reset color

//log level enum
typedef enum{
	MSLOG_DEBUG = 0,
	MSLOG_INFO,
	MSLOG_WARN,
	MSLOG_ERROR,
	MSLOG_FATAL,
	MSLOG_LEVEL_MAX
}mslog_level_t;

//flush mode
typedef enum{
	MSLOG_FLUSH_BATCH = 0,
	MSLOG_FLUSH_REAL_TIME
}mslog_flush_mode_t;

//log global config struct
typedef struct{
	char log_path[256];
	FILE *log_fp;
	mslog_level_t log_level;
	mslog_flush_mode_t flush_mode;
	size_t max_file_size;
	int max_file_count;
	size_t total_write_bytes;
	int total_flush_time;
	int rotate_check_cnt;
	pthread_mutex_t log_mutex;
	int enable_console_color;
    
}mslog_global_t;

extern mslog_global_t g_mslog;

//multi-thread & single-thread
#ifdef MULTI_THREAD
#define LOCK_STAT()			pthread_mutex_lock(&g_stat_mutex)
#define UNLOCK_STAT()		pthread_mutex_unlock(&g_stat_mutex)
#define M_POOL_INIT()		mslog_mem_pool_init()
#define M_POOL_DEINIT()		mslog_mem_pool_deinit()
#define M_MEM_ALLOC(s)		mslog_mem_pool_alloc(s)
#define M_MEM_FREE(p)		mslog_mem_pool_free(p)
#else
#define LOCK_STAT()			do{} while (0)
#define UNLOCK_STAT()		do{} while (0)
#define M_POOL_INIT()		do{} while (0)
#define M_POOL_DEINIT()		do{} while (0)
#define M_MEM_ALLOC(s)		malloc(s)
#define M_MEM_FREE(p)		free(p)
#endif

//default config marco
#define MSLOG_ROTATE_CHECK_MAX			( 100 )
#define MSLOG_LOG_BUF_SIZE				( 2048 )

//log output interface
int mslog_init_default(const char *log_path, mslog_level_t log_level, 
        size_t max_file_size, int max_file_count, mslog_flush_mode_t flush_mode);
void mslog_deinit(void);
void mslog_log(mslog_level_t level, const char *tag, const char *file, int line, 
        const char *func, const char *fmt, ...);

//log marco define
#define MSLOG_DEBUG(tag, fmt, ...)\
	mslog_log(MSLOG_DEBUG, tag, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define MSLOG_INFO(tag, fmt, ...)\
	mslog_log(MSLOG_INFO, tag, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define MSLOG_WARN(tag, fmt, ...)\
	mslog_log(MSLOG_WARN, tag, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define MSLOG_ERROR(tag, fmt, ...)\
	mslog_log(MSLOG_ERROR, tag, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define MSLOG_FATAL(tag, fmt, ...)\
	mslog_log(MSLOG_FATAL, tag, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif//__MSLOG_H__
