#define _POSIX_C_SOURCE 200112L
#include "mslog.h"

mslog_global_t g_mslog ={
	.log_path = MSLOG_DEFAULT_LOG_PATH,
	.log_level = MSLOG_DEFAULT_LOG_LEVEL,
	.rotate_size = MSLOG_DEFAULT_ROTATE_SIZE,
	.rotate_num = MSLOG_DEFAULT_ROTATE_NUM,
	.flush_mode = MSLOG_DEFAULT_FLUSH_MODE,
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
	.log_buf_pool = {
		.free_list = NULL,
		.buf_size = MSLOG_DEFAULT_LOG_BUF_SIZE,
		.pool_size = MSLOG_DEFAULT_LOG_POOL_SIZE,
		.pool_mutex = PTHREAD_MUTEX_INITIALIZER,
	},
	.log_fp = NULL,
	.flush_thread = 0,
	.flush_running = 0,
	.time_cache = {0},
	.last_time = 0,
	.rotate_check_cnt = 0
};

//stat cnt var
static size_t g_total_write_bytes = 0;
static size_t g_total_flush_time = 0;
static size_t g_flush_count = 0;

//tool func pre-define
static int tag_match(const char *tag);
static void update_time_cache(void);
static void log_rotate(void);
static void do_flush(void);
