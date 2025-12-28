#define _POSIX_C_SOURCE 200112L
#include "mslog_mem_pool.h"
#include "mslog_thread.h"
#include "mslog_utils.h"
#include <stdio.h>
#include <time.h>
#include "mslog.h"

mslog_global_t g_mslog ={
	.log_path = MSLOG_DEFAULT_LOG_PATH,
	.log_level = MSLOG_DEFAULT_LOG_LEVEL,
	.rotate_size = MSLOG_DEFAULT_ROTATE_SIZE,
	.rotate_num = MSLOG_DEFAULT_ROTATE_NUM,
	.flush_mode = MSLOG_DEFAULT_FLUSH_MODE,
    .enable_console_color = MSLOG_DEFAULT_ENABLE_CONSOLE_COLOR,
	.batch_threshold = MSLOG_DEFAULT_MIN_BATCH_THRESHOLD,
	.max_batch_threshold = MSLOG_DEFAULT_MAX_BATCH_THRESHOLD,
	.min_batch_threshold = MSLOG_DEFAULT_MIN_BATCH_THRESHOLD,
	.log_bytes_per_sec = 0,
	.last_stat_time = 0,
	.curr_buf_used = 0,
	.io_buf = NULL,
	.mmap_buf = MAP_FAILED,
	.mmap_size = MSLOG_DEFAULT_MMAP_INIT_SIZE,
	.mmap_offset = 0,
	.mmap_min_file_size = MSLOG_DEFAULT_MMAP_MIN_FILE_SIZE,
	.log_fp = NULL,
	.time_cache = {0},
	.last_time = 0,
	.rotate_check_cnt = 0,
	//.tag_filter = ""
};

//stat cnt var
size_t g_total_write_bytes = 0;
size_t g_total_flush_time = 0;
size_t g_flush_count = 0;

//-----------func pre-define only use in core module--------------------//
static void update_write_stat(size_t write_len, mslog_level_t level);
static int mslog_mmap_init(void);
static void mslog_mmap_destroy(void);
static int mslog_mmap_write(const char *buf, size_t len);
static void mslog_output(mslog_level_t level, const char *log_content, size_t len);
//----------------------------write count------------------------------//
static void update_write_stat(size_t write_len, mslog_level_t level){
	time_t now = time(NULL);
	mslog_lock(level);
	if ( now != g_mslog.last_stat_time ){
		g_mslog.log_bytes_per_sec = write_len;
		g_mslog.last_stat_time = now;
		if ( g_mslog.log_bytes_per_sec > ( g_mslog.min_batch_threshold / 2 ) ){
			g_mslog.batch_threshold = g_mslog.max_batch_threshold;
		}
		else
		{
			g_mslog.batch_threshold = g_mslog.min_batch_threshold;
		}
	}
	else
	{
		g_mslog.log_bytes_per_sec += write_len;
	}
	g_total_write_bytes += write_len;
	mslog_unlock(level);
}

//------------mmap---------------//
static int mslog_mmap_init(void){
	if ( g_mslog.log_fp == NULL ){
		return -1;
	}

	int fd = fileno(g_mslog.log_fp);
	off_t file_size = lseek(fd, 0, SEEK_END);
	if ( file_size < 0 ){
		return -2;
	}

	if ( (size_t)file_size < g_mslog.mmap_min_file_size ){
		return -2;
	}

	g_mslog.mmap_buf = mmap(NULL, g_mslog.mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( g_mslog.mmap_buf == MAP_FAILED ){
		return -1;
	}
	g_mslog.mmap_offset = file_size;
	return 0;
}

static void mslog_mmap_destroy(void){
	if ( g_mslog.mmap_buf != MAP_FAILED ){
		msync(g_mslog.mmap_buf, g_mslog.mmap_offset, MS_SYNC);
		munmap(g_mslog.mmap_buf, g_mslog.mmap_size);
		g_mslog.mmap_buf = MAP_FAILED;
		g_mslog.mmap_offset = 0;
	}
}

static int mslog_mmap_write(const char *buf, size_t len){
	if ( g_mslog.mmap_buf == MAP_FAILED || len == 0 ){
		return -1;
	}

	if ( g_mslog.mmap_offset + len > g_mslog.mmap_size ){
		mslog_mmap_destroy();

		g_mslog.mmap_size *= 2;
		if ( mslog_mmap_init() != 0 ){
			return -1;
		}
	}

	memcpy((char *)g_mslog.mmap_buf + g_mslog.mmap_offset, buf, len);
	g_mslog.mmap_offset += len;

	msync(g_mslog.mmap_buf, g_mslog.mmap_offset, MS_ASYNC);
	return 0;
}
//------------------log input file write + console print--------------//
static void mslog_output(mslog_level_t level, const char *log_content, size_t len){
    if ( g_mslog.log_fp != NULL ){
        if ( mslog_mmap_write(log_content, len) != 0 ) {
            fwrite(log_content, 1, len, g_mslog.log_fp);
        }
        fflush(g_mslog.log_fp);
        fsync(fileno(g_mslog.log_fp));
    }

    if ( g_mslog.enable_console_color ){
        fprintf(stdout, "%s%s%s", mslog_get_level_color(level), log_content, MSLOG_COLOR_RESET);
    }
    else {
        fprintf(stdout, "%s", log_content);
    }
    fflush(stdout);
}
//---------------------output interface init--------------------------//
int mslog_init(const char *log_path, mslog_level_t log_level, size_t rotate_size, 
        int rotate_num, mslog_flush_mode_t flush_mode, int enable_console_color){
	for ( int i = 0; i < MSLOG_LEVEL_MAX; i++ ){
		pthread_mutex_init(&g_mslog.level_mutex[i], NULL);
	}

	if ( log_path != NULL ){
		strncpy(g_mslog.log_path, log_path, ( sizeof(g_mslog.log_path) - 1 ));
		g_mslog.log_path[sizeof(g_mslog.log_path) - 1] = '\0';
	}
	if ( log_level >= 0 && log_level < MSLOG_LEVEL_MAX ){
		g_mslog.log_level = log_level;
	}
	if ( rotate_size > 0 ){
		g_mslog.rotate_size = rotate_size;
	}
	if ( rotate_num > 0 ){
		g_mslog.rotate_num = rotate_num;
	}
	g_mslog.flush_mode = flush_mode;
    g_mslog.enable_console_color = ( enable_console_color == 1 ) ? 1 : 0;

	g_mslog.io_buf = (char *)malloc(g_mslog.max_batch_threshold);
	if ( g_mslog.io_buf == NULL ){
		goto err_mutex;
	}

	if ( mslog_mem_pool_init(&g_mslog.log_buf_pool, MSLOG_DEFAULT_MIN_BATCH_THRESHOLD, 10) != 0 ){
		goto err_io_buf;
	}

	g_mslog.log_fp = fopen(g_mslog.log_path, "a");
	if ( g_mslog.log_fp == NULL ){
		goto err_mem_pool;
	}

	mslog_mmap_init();

	if ( g_mslog.flush_mode == MSLOG_FLUSH_BATCH ){
		if ( mslog_thread_init(&g_mslog.flush_thread, MSLOG_FLUSH_INTERVAL, 
                    MSLOG_ROTATE_CHECK_CNT_MAX, &g_mslog) != 0 ){
			goto err_log_fp;
		}
	}

	mslog_utils_update_time_cache(g_mslog.time_cache, &g_mslog.last_time, sizeof(g_mslog.time_cache));
	return 0;

err_log_fp:
	fclose(g_mslog.log_fp);
	g_mslog.log_fp = NULL;
	mslog_mmap_destroy();

err_mem_pool:
	mslog_mem_pool_destroy(&g_mslog.log_buf_pool);

err_io_buf:
	free(g_mslog.io_buf);
	g_mslog.io_buf = NULL;

err_mutex:
	for ( int i = 0; i < MSLOG_LEVEL_MAX; i++ ){
		pthread_mutex_destroy(&g_mslog.level_mutex[i]);
	}
	return -1;
}

//-------------------------output interface deinit-------------------------//
void mslog_deinit(void){
    if ( g_mslog.log_fp != NULL ) {
        if ( g_mslog.curr_buf_used > 0 && g_mslog.io_buf != NULL) {
            fwrite(g_mslog.io_buf, 1, g_mslog.curr_buf_used, g_mslog.log_fp);
            g_mslog.curr_buf_used = 0;
        }
        fflush(g_mslog.log_fp);
        fsync(fileno(g_mslog.log_fp));
    }

	if (g_mslog.flush_mode == MSLOG_FLUSH_BATCH) {
	    mslog_thread_destroy(&g_mslog.flush_thread);
	}

	mslog_mmap_destroy();

	if ( g_mslog.log_fp != NULL ){
		fclose(g_mslog.log_fp);
		g_mslog.log_fp = NULL;
	}

	mslog_mem_pool_destroy(&g_mslog.log_buf_pool);

	if ( g_mslog.io_buf != NULL ){
		free(g_mslog.io_buf);
		g_mslog.io_buf = NULL;
	}

	for ( int i = 0; i < MSLOG_LEVEL_MAX; i++ ){
		pthread_mutex_destroy(&g_mslog.level_mutex[i]);
	}
}

//-------------------output interface log write----------------------//
void mslog_log(mslog_level_t level, const char *tag, const char *file, int line, 
        const char *func, const char *fmt, ...){
	if ( level < 0 || level >= MSLOG_LEVEL_MAX || level < g_mslog.log_level ){
		return;
	}

	if ( !mslog_utils_tag_match(tag, g_mslog.log_path) ){
		return;
	}

	mslog_buf_node_t *log_node = mslog_mem_pool_alloc(&g_mslog.log_buf_pool);
	if ( log_node == NULL ){
		return;
	}
	char *log_buf = log_node->buf;
	size_t buf_size = log_node->size;
	size_t len = 0;

	const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

	mslog_utils_update_time_cache(g_mslog.time_cache, &g_mslog.last_time, sizeof(g_mslog.time_cache));
	len += snprintf(log_buf + len, buf_size - len,
			"[%s] [%s] [%s] %s:%d:%s",
			g_mslog.time_cache,
			level_str[level],
			tag ? tag : "NULL",
			file, line, func);

	va_list args;
	va_start(args, fmt);
	len += vsnprintf(log_buf + len, buf_size - len, fmt, args);
	va_end(args);

	if ( len < buf_size - 1 ){
		log_buf[len++] = '\n';
		log_buf[len] = '\0';
	}

	mslog_lock(level);
	if ( g_mslog.flush_mode == MSLOG_FLUSH_REAL_TIME ){
		mslog_output(level, log_buf, len);
		update_write_stat(len, level);
        g_mslog.rotate_check_cnt++;
        if ( g_mslog.rotate_check_cnt >= MSLOG_ROTATE_CHECK_CNT_MAX ) {
            mslog_utils_log_rotate(&g_mslog);
            g_mslog.rotate_check_cnt = 0;
        }
	}
	else
	{
		if ( g_mslog.curr_buf_used + len > g_mslog.batch_threshold ){
			mslog_thread_destroy(&g_mslog.flush_thread);
            mslog_thread_init(&g_mslog.flush_thread, MSLOG_FLUSH_INTERVAL, MSLOG_ROTATE_CHECK_CNT_MAX, &g_mslog);
		}
		if ( g_mslog.curr_buf_used + len <= g_mslog.max_batch_threshold ){
			memcpy(g_mslog.io_buf + g_mslog.curr_buf_used, log_buf, len);
			g_mslog.curr_buf_used += len;
			update_write_stat(len, level);
		}

		if ( (double)( g_mslog.curr_buf_used / g_mslog.max_batch_threshold > 0.8 ) ){
			mslog_thread_destroy(&g_mslog.flush_thread);
            mslog_thread_init(&g_mslog.flush_thread, MSLOG_FLUSH_INTERVAL, MSLOG_ROTATE_CHECK_CNT_MAX, &g_mslog);
		}
	}
	mslog_unlock(level);

	mslog_mem_pool_free(&g_mslog.log_buf_pool, log_node);
}

//------------------------output interface get stat-------------------//
void mslog_get_stats(size_t *total_write_bytes, size_t *avg_flush_time, size_t *buf_usage_rate){
	if ( total_write_bytes != NULL ){
		*total_write_bytes = g_total_write_bytes;
	}
	if ( avg_flush_time != NULL ){
		*avg_flush_time = ( g_flush_count == 0 ) ? 0 : ( g_total_flush_time / g_flush_count );
	}
	if ( buf_usage_rate != NULL ){
		*buf_usage_rate = ( g_mslog.max_batch_threshold == 0 ) ? 0 : ( ( g_mslog.curr_buf_used * 100 ) / g_mslog.max_batch_threshold );
	}
}
