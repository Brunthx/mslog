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
#include<sys/stat.h>
#include<limits.h>

//global config marco
#define MSLOG_MAX_LOG_LEN               ( 2048 )
#define MSLOG_MAX_PATH_LEN              PATH_MAX
#define MSLOG_IO_BUF_SIZE		( 1024 * 8)
#define MSLOG_FLUSH_INTERVAL		( 1 )
#define MSLOG_ROTATE_CHECK_CNT		( 100 )
#define MSLOG_DEFAULT_ROTATE_SIZE       ( 10 * 1024 * 1024 )
#define MSLOG_DEFAULT_ROTATE_NUM        ( 5 )

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

//line/func name's marco
#define MSLOG_FILE_LINE_FMT	"[%s:%d:%s:]"
#define MSLOG_FILE_LINE_FUNC	__FILE__, __LINE__, __func__

//log output marco
#define MSLOG(level, tag, fmt, ...)\
	do{\
		if ( level < mslog_get_level() ) break;\
		mslog_core(level, tag, MSLOG_FILE_LINE_FUNC, fmt, ##__VA_ARGS__);\
	}while(0)

//simple marco
#define MSLOG_DBG(tag, fmt, ...) MSLOG(MSLOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define MSLOG_INFO(tag, fmt, ...) MSLOG(MSLOG_INFO, tag, fmt, ##__VA_ARGS__)
#define MSLOG_WARN(tag, fmt, ...) MSLOG(MSLOG_WARN, tag, fmt, ##__VA_ARGS__)
#define MSLOG_ERR(tag, fmt, ...) MSLOG(MSLOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define MSLOG_FATAL(tag, fmt, ...) MSLOG(MSLOG_FATAL, tag, fmt, ##__VA_ARGS__)

//global config interface
void mslog_set_flush_mode(mslog_flush_mode_t mode);//setting auto flush mode
void mslog_flush(void);//setting flush
void mslog_set_level(mslog_level_t level);//setting log level
void mslog_set_tag_filter(const char *filter);//setting log char filter
int mslog_init(const char *log_path, size_t rotate_size, int rotate_num);//module initial
void mslog_deinit(void);//module delete

//log output interface
void mslog_core(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...);

mslog_level_t mslog_get_level(void);//get log level
#endif//MSLOG_H
