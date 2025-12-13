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

//log level enum
typedef enum{
	MSLOG_DEBUG = 0,
	MSLOG_INFO,
	MSLOG_WARN,
	MSLOG_ERROR,
	MSLOG_FATAL,
	MSLOG_LEVEL_MAX
}mslog_level_t;

//global config marco
#define MSLOG_MAX_TIME_LEN		( 32 )
#define MSLOG_MAX_LOG_LEN		( 2048 )
#define MSLOG_MAX_PATH_LEN		PATH_MAX
#define MSLOG_DEFAULT_ROTATE_SIZE	( 10 * 1024 * 1024 )
#define MSLOG_DEFAULT_ROTATE_NUM	( 5 )

//color marco only for linux
#ifdef __linux__
#define MS_COLOR_ERROR	"\033[031m"//red: error
#define MS_COLOR_DEBUG	"\033[032m"//green: debug
#define MS_COLOR_WARN	"\033[033m"//yellow: warn
#define MS_COLOR_INFO	"\033[034m"//blue: info
#define MS_COLOR_FATAL	"\033[041m"
#define MS_COLOR_RESET	"\033[0m"
#else
#define MS_COLOR_ERROR	""
#define MS_COLOR_DEBUG	""
#define MS_COLOR_WARN	""
#define MS_COLOR_INFO	""
#define MS_COLOR_FATAL	""
#define MS_COLOR_RESET	""
#endif

//line/func name's marco
#define MSLOG_FILE_LINE_FMT	"[%s:%d:%s:]"
#define MSLOG_FILE_LINE_FUNC	__FILE__, __LINE__, __func__

//log output marco
#define MSLOG(level, tag, fmt, ...)\
	mslog_core(level, tag, MSLOG_FILE_LINE_FUNC, fmt, ##__VA_ARGS__)

//simple marco
#define MSLOG_DBG(tag, fmt, ...) MSLOG(MSLOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define MSLOG_INFO(tag, fmt, ...) MSLOG(MSLOG_INFO, tag, fmt, ##__VA_ARGS__)
#define MSLOG_WARN(tag, fmt, ...) MSLOG(MSLOG_WARN, tag, fmt, ##__VA_ARGS__)
#define MSLOG_ERR(tag, fmt, ...) MSLOG(MSLOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define MSLOG_FATAL(tag, fmt, ...) MSLOG(MSLOG_FATAL, tag, fmt, ##__VA_ARGS__)

//global config interface
void mslog_set_level(mslog_level_t level);//setting log level
void mslog_set_tag_filter(const char *filter);//setting log char filter
void mslog_enable_line_info(int enable);//enable or disable print line or func
void mslog_enable_stdout(int enable);//enable or disable stdout
void mslog_init(const char *log_path, size_t rotate_size, int rotate_num);//module initial
void mslog_deinit(void);//module delete

//log output interface
void mslog_core(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...);
#endif//MSLOG_H
