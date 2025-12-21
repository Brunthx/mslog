#ifndef MSLOG_H
#define MSLOG_H

#define _POSIX_C_SOURCE 200112L
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<stdarg.h>
#include<time.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<signal.h>
#include<fcntl.h>

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
	MSLOG_FLUSH_REALTIME
}mslog_flush_mode_t;

//mem pool buf struct
typedef struct mslog_buf_node{
	char *buf;
	size_t size;
	struct mslog_buf_node *next;
}mslog_buf_node_t;

//mem pool struct
typedef struct{
	mslog_buf_node_t *free_list;
	size_t buf_size;
	size_t pool_size;
	pthread_mutex_t pool_mutex;
}mslog_mem_pool_t;

//log global config struct
typedef struct{
	char log_path[256];
	mslog_level_t log_level;
	size_t rotate_size;
	int rotate_num;
	mslog_flush_mode_t flush_mode;

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

	FILE *log_fp;
	pthread_mutex_t level_mutex[MSLOG_LEVEL_MAX];
	pthread_t flush_thread;
	volatile int flush_running;
	char time_cache[32];
	time_t last_time;
	int rotate_check_cnt;
	char tag_filter[1024];
}mslog_global_t;

//default config marco
#define MSLOG_DEFAULT_LOG_PATH			( "./mslog.log" )
#define MSLOG_DEFAULT_LOG_LEVEL			( MSLOG_INFO )
#define MSLOG_DEFAULT_ROTATE_SIZE		( 10 * 1024 * 1024 )
#define MSLOG_DEFAULT_ROTATE_NUM		( 5 )
#define MSLOG_DEFAULT_FLUSH_MODE		( MSLOG_FLUSH_BATCH )
#define MSLOG_DEFAULT_MIN_BATCH_THRESHOLD	( 4096 )//kb
#define MSLOG_DEFAULT_MAX_BATCH_THRESHOLD	( 16384 )//16kb
#define MSLOG_DEFAULT_IO_BUF_SIZE		( 16384 )//16kb
#define MSLOG_DEFAULT_MMAP_MIN_FILE_SIZE	( 1 * 1024 * 1024 )//1mb
#define MSLOG_DEFAULT_MMAP_INIT_SIZE		( 4 * 1024 * 1024 )//4mb
#define MSLOG_DEFAULT_LOG_BUF_POOL_SIZE		( 10 )
#define MSLOG_DEFAULT_LOG_BUF_SIZE		( 4096 )
#define MSLOG_FLUSH_INTERVAL			( 1 )
#define MSLOG_ROTATE_CHECK_CNT_MAX		( 100 )

//log output interface
int mslog_init(const char *log_path, mslog_level_t log_level, size_t rotate_size, int rotate_num, mslog_flush_mode_t flush_mode);
void mslog_deinit(void);
void mslog_log(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...);
void mslog_get_stats(size_t *total_write_bytes, size_t *avg_flush_time, size_t *bu_usage_rate);

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
#endif//MSLOG_H
