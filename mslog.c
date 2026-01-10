#define _POSIX_C_SOURCE 200112L
#include "mslog_include.h"

mslog_global_t g_mslog = {
	.log_fp = NULL,
	.log_level = MSLOG_INFO,
	.flush_mode = MSLOG_FLUSH_REAL_TIME,
	.max_file_size = 1024 * 1024 * 200,
	.max_file_count = 5,
	.total_write_bytes = 0,
	.total_flush_time = 0,
	.rotate_check_cnt = 0,
	.enable_console_color = 1,
	.batch_buf = NULL,
	.batch_buf_total = 0,
	.batch_buf_used = 0,
	.log_mutex = PTHREAD_MUTEX_INITIALIZER 
};

#ifdef MULTI_THREAD
static pthread_mutex_t g_stat_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static const char *mslog_level2str(mslog_level_t level){
	switch (level)
	{
	case MSLOG_DEBUG:
		return "DEBUG";
	case MSLOG_INFO:
		return "INFO";
	case MSLOG_WARN:
		return "WARN";
	case MSLOG_ERROR:
		return "ERROR";
	case MSLOG_FATAL:
		return "FATAL";
	default:
		return "UNKNOWN";
	}
}

static const char *mslog_level2color(mslog_level_t level){
	switch (level)
	{
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

static void mslog_update_stat(size_t len){
	LOCK_STAT();
	g_mslog.total_write_bytes += len;
	g_mslog.total_flush_time++;
	g_mslog.rotate_check_cnt++;
	UNLOCK_STAT();
}

static void mslog_output(mslog_level_t level, const char *log_buf, size_t len){
	if ( len == 0 || log_buf == NULL || len > MSLOG_LOG_BUF_SIZE )
	{
		return;
	}

	if ( g_mslog.enable_console_color )
	{
		fprintf(stdout, "%s%s%s", mslog_level2color(level), log_buf, MSLOG_COLOR_RESET);
	}
	else
	{
		fprintf(stdout, "%s", log_buf);
	}
	fflush(stdout);
}

static void mslog_batch_flush(void){
	if ( !M_MEM_IS_VALID(g_mslog.batch_buf) || g_mslog.log_fp == NULL || g_mslog.batch_buf_used == 0 ) 
	{
		return;
	}


	size_t write_len = g_mslog.batch_buf_used;
    if (write_len == 0 || write_len > g_mslog.batch_buf_total) {
        g_mslog.batch_buf_used = 0;
        return;
    }
	
	fwrite(g_mslog.batch_buf, 1, g_mslog.batch_buf_used, g_mslog.log_fp);
	fflush(g_mslog.log_fp);
	if (fileno(g_mslog.log_fp) > 0)
	{
        fsync(fileno(g_mslog.log_fp));
    }

	memset(g_mslog.batch_buf, 0, g_mslog.batch_buf_total);
	g_mslog.batch_buf_used = 0;
}

void mslog_keep_alive(void){
    if ( g_mslog.log_fp == NULL ) 
    {
        return;
    }

    static time_t last_alive = 0;
    time_t now = time(NULL);
    if ( now - last_alive < 60 )
    {
        return;
    }
    last_alive = now;

    fputc('\0', g_mslog.log_fp);
    fflush(g_mslog.log_fp);
    fsync(fileno(g_mslog.log_fp));
}

int mslog_init_default(const char *log_path, mslog_level_t log_level, size_t max_file_size, int max_file_count, mslog_flush_mode_t flush_mode){
	if (log_path == NULL || strlen(log_path) == 0 || max_file_count <= 0)
	{
        fprintf(stderr, "[mslog] init failed: invalid param\n");
        return -1;
    }
	
	M_POOL_INIT();

	strncpy(g_mslog.log_path, log_path, ( sizeof(g_mslog.log_path) - 1 ));
	g_mslog.log_level = log_level;
	g_mslog.flush_mode = flush_mode;
	g_mslog.max_file_size = max_file_size;
	g_mslog.max_file_count = max_file_count;

	g_mslog.log_fp = fopen(g_mslog.log_path, "a+");
	if ( g_mslog.log_fp == NULL )
	{
		fprintf(stderr, "[mslog] open log file failed: %s\n", strerror(errno));
        M_POOL_DEINIT();
		return -2;
	}

	if ( g_mslog.flush_mode == MSLOG_FLUSH_BATCH )
	{
		g_mslog.batch_buf_total = MSLOG_BATCH_BUF_SIZE;
		g_mslog.batch_buf = (char *)M_MEM_ALLOC(g_mslog.batch_buf_total);
		if ( g_mslog.batch_buf == NULL)
		{
			g_mslog.batch_buf = (char *)malloc(g_mslog.batch_buf_total);
		}
		
		if ( g_mslog.batch_buf == NULL )
		{
			fprintf(stderr, "[mslog] batch buf alloc failed, downgrade to realtime flush\n");
            g_mslog.flush_mode = MSLOG_FLUSH_REAL_TIME;
            g_mslog.batch_buf_total = 0;
            g_mslog.batch_buf_used = 0;
		}
		else
		{
			memset(g_mslog.batch_buf, 0, g_mslog.batch_buf_total);
			g_mslog.batch_buf_used = 0;
		}
	}
	fprintf(stdout, "[mslog] init success | flush mode: %s | log path: %s\n",
            flush_mode == MSLOG_FLUSH_BATCH ? "BATCH" : "REALTIME", log_path);
	return 0;
}

void mslog_deinit(void){
	if ( g_mslog.flush_mode == MSLOG_FLUSH_BATCH )
	{
		mslog_batch_flush();
		if ( M_MEM_IS_VALID(g_mslog.batch_buf) )
		{
			M_MEM_FREE(g_mslog.batch_buf);
			g_mslog.batch_buf = NULL;
		}
		g_mslog.batch_buf_total = 0;
		g_mslog.batch_buf_used = 0;
	}
	

	if ( g_mslog.log_fp != NULL)
	{
		fflush(g_mslog.log_fp);
		if (fileno(g_mslog.log_fp) > 0)
		{
            fsync(fileno(g_mslog.log_fp));
        }
		fclose(g_mslog.log_fp);
		g_mslog.log_fp = NULL;
	}
	M_POOL_DEINIT();
#ifdef MULTI_THREAD
    if (pthread_mutex_trylock(&g_mslog.log_mutex) == 0)
	{
        pthread_mutex_unlock(&g_mslog.log_mutex);
        pthread_mutex_destroy(&g_mslog.log_mutex);
    }
#endif
}

void mslog_log(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...){
	if ( level < g_mslog.log_level || level >= MSLOG_LEVEL_MAX )
	{
		return;
	}
	
	if ( tag == NULL || fmt == NULL )
	{
		return;
	}

	char *log_buf = (char *)M_MEM_ALLOC((size_t)MSLOG_LOG_BUF_SIZE);
	if ( !M_MEM_IS_VALID(log_buf) )
	{
		return;
	}

	va_list args;
	char time_buf[32] = {0};

	tag = (tag == NULL) ? "TEST" : tag;
	file = (file == NULL) ? "test_mslog.c" : file;
	func = (func == NULL) ? "main" : func;

	mslog_utils_get_time_str(time_buf, sizeof(time_buf));
	int head_len = snprintf(log_buf, MSLOG_LOG_BUF_SIZE,
                            "[%s] [%s] [%s] %s:%d:%s ",
                            time_buf, mslog_level2str(level), tag, file, line, func);
	if ( head_len < 0 || head_len >= MSLOG_LOG_BUF_SIZE - 1 )
	{
		M_MEM_FREE(log_buf);
		return;
	}
	
	va_start(args, fmt);
	int content_len = vsnprintf(log_buf + head_len, MSLOG_LOG_BUF_SIZE - head_len, fmt, args);
	va_end(args);
	if ( content_len <= 0 || ( head_len + content_len >= MSLOG_LOG_BUF_SIZE - 1 ) )
	{
		M_MEM_FREE(log_buf);
		return;
	}

	size_t total_len = head_len +content_len + 1;
	if (total_len >= MSLOG_LOG_BUF_SIZE)
	{
		total_len = MSLOG_LOG_BUF_SIZE - 1;
	}
	log_buf[total_len -1] = '\n';
	log_buf[total_len] = '\0';

	pthread_mutex_lock(&g_mslog.log_mutex);
	if ( g_mslog.flush_mode == MSLOG_FLUSH_REAL_TIME )
	{
		mslog_output(level, log_buf, total_len);
		mslog_update_stat(total_len);

		if ( g_mslog.log_fp )
		{
			fwrite(log_buf, 1, total_len, g_mslog.log_fp);
			fflush(g_mslog.log_fp);
			fsync(fileno(g_mslog.log_fp));
		}
		
	}
	else if ( g_mslog.flush_mode == MSLOG_FLUSH_BATCH )
	{
		if ( total_len == 0 || !M_MEM_IS_VALID(log_buf) )
		{
			goto __EXIT_LOCK;
		}
		if ( g_mslog.log_fp == NULL || !M_MEM_IS_VALID(g_mslog.batch_buf) )
		{
			mslog_output(level, log_buf, total_len);
			mslog_update_stat(total_len);
			goto __EXIT_LOCK;
		}
		
		if (total_len >= g_mslog.batch_buf_total) 
		{
            fwrite(log_buf, 1, total_len, g_mslog.log_fp);
            fflush(g_mslog.log_fp);
            mslog_output(level, log_buf, total_len);
            mslog_update_stat(total_len);
            goto __EXIT_LOCK;
        }

		size_t free_size = g_mslog.batch_buf_total - g_mslog.batch_buf_used;
		if ( free_size > total_len )
		{
			if ( M_MEM_IS_VALID(g_mslog.batch_buf) && M_MEM_IS_VALID(log_buf) )
			{
				memcpy(g_mslog.batch_buf + g_mslog.batch_buf_used, log_buf, total_len);
				g_mslog.batch_buf_used += total_len;
			}
		}
		else
		{
			mslog_batch_flush();
			if ( M_MEM_IS_VALID(g_mslog.batch_buf) && M_MEM_IS_VALID(log_buf) )
			{
				memcpy(g_mslog.batch_buf, log_buf, total_len);
	            g_mslog.batch_buf_used = total_len;
			}
		}
		mslog_output(level, log_buf, total_len);
        mslog_update_stat(total_len);
	}
	
	//log rotate check
	if ( g_mslog.rotate_check_cnt >= MSLOG_ROTATE_CHECK_MAX )
	{
		if ( mslog_utils_get_file_size(g_mslog.log_path) >= g_mslog.max_file_size )
		{
			mslog_utils_log_rotate(g_mslog.log_path, g_mslog.max_file_count);
		}
		g_mslog.rotate_check_cnt = 0;
	}
	
__EXIT_LOCK:
	pthread_mutex_unlock(&g_mslog.log_mutex);
	M_MEM_FREE(log_buf);
}
