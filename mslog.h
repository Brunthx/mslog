#ifndef __MSLOG_H__
#define __MSLOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

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
struct mslog_global{
	char log_path[256];
	mslog_level_t log_level;
	size_t rotate_size;
	int rotate_num;
	mslog_flush_mode_t flush_mode;
    int enable_console_color;

	size_t batch_threshold;
	size_t max_batch_threshold;
	size_t min_batch_threshold;
	size_t log_bytes_per_sec;
	time_t last_stat_time;
	size_t curr_buf_used;
	char *io_buf;

	void *mmap_buf;
	size_t mmap_size;
	size_t mmap_offset;
	size_t mmap_min_file_size;

	mslog_mem_pool_t log_buf_pool;
    mslog_thread_ctx_t flush_thread;

	FILE *log_fp;
	pthread_mutex_t level_mutex[MSLOG_LEVEL_MAX];
	char time_cache[32];
	time_t last_time;
	int rotate_check_cnt;
	//char tag_filter[1024];
};

typedef struct mslog_global mslog_global_t;

//default config marco
#define MSLOG_DEFAULT_LOG_PATH			    ( "./mslog.log" )
#define MSLOG_DEFAULT_LOG_LEVEL			    ( MSLOG_INFO )
#define MSLOG_DEFAULT_ROTATE_SIZE		    ( 10 * 1024 * 1024 )
#define MSLOG_DEFAULT_ROTATE_NUM		    ( 5 )
#define MSLOG_DEFAULT_FLUSH_MODE		    ( MSLOG_FLUSH_BATCH )
#define MSLOG_DEFAULT_ENABLE_CONSOLE_COLOR  ( 1 )

#define MSLOG_DEFAULT_MIN_BATCH_THRESHOLD	( 1 * 1024 * 1024 )//1mb
#define MSLOG_DEFAULT_MAX_BATCH_THRESHOLD	( 4 * 1024 * 1024 )//4mb
#define MSLOG_DEFAULT_IO_BUF_SIZE           ( 4 * 1024 * 1024 )//4mb

#define MSLOG_DEFAULT_MMAP_MIN_FILE_SIZE	( 1 * 1024 * 1024 )//1mb
#define MSLOG_DEFAULT_MMAP_INIT_SIZE		( 4 * 1024 * 1024 )//4mb


#define MSLOG_FLUSH_INTERVAL			    ( 1 )
#define MSLOG_ROTATE_CHECK_CNT_MAX		    ( 100 )

//log output interface
int mslog_init(const char *log_path, mslog_level_t log_level, size_t rotate_size, 
        int rotate_num, mslog_flush_mode_t flush_mode, int enable_console_color);
static inline int mslog_init_default(const char *log_path, mslog_level_t log_level, 
        size_t rotate_size, int rotate_num, mslog_flush_mode_t flush_mode){
    return mslog_init(log_path, log_level, rotate_size, rotate_num, flush_mode, MSLOG_DEFAULT_ENABLE_CONSOLE_COLOR);
}
void mslog_deinit();
void mslog_log(mslog_level_t level, const char *tag, const char *file, int line, 
        const char *func, const char *fmt, ...);
void mslog_get_stats(size_t *total_write_bytes, size_t *avg_flush_time, size_t *buf_usage_rate);

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

//static inline func
static inline mslog_level_t mslog_get_level(void){
	extern mslog_global_t g_mslog;
	return g_mslog.log_level;
}

static inline void mslog_lock(mslog_level_t level){
	extern mslog_global_t g_mslog;
	if ( level >= 0 && level < MSLOG_LEVEL_MAX ){
		pthread_mutex_lock(&g_mslog.level_mutex[level]);
	}
}

static inline void mslog_unlock(mslog_level_t level){
	extern mslog_global_t g_mslog;
	if ( level >= 0 && level < MSLOG_LEVEL_MAX ){
		pthread_mutex_unlock(&g_mslog.level_mutex[level]);
	}
}

static inline const char *mslog_get_level_color(mslog_level_t level){
    switch (level) {
        case MSLOG_DEBUG:
            return MSLOG_COLOR_DEBUG;
        case MSLOG_INFO:
            return MSLOG_COLOR_INFO;
        case MSLOG_WARN:
            return MSLOG_COLOR_WARN;
        case MSLOG_ERROR:
            return MSLOG_COLOR_ERROR;
        case MSLOG_FATAL:
            return MSLOG_COLOR_FATAL;
        default:
            return MSLOG_COLOR_RESET;
    }
}
#endif//__MSLOG_H__
