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
		.pool_size = MSLOG_DEFAULT_LOG_BUF_POOL_SIZE,
		.pool_mutex = PTHREAD_MUTEX_INITIALIZER,
	},
	.log_fp = NULL,
	.flush_thread = 0,
	.flush_running = 0,
	.time_cache = {0},
	.last_time = 0,
	.rotate_check_cnt = 0,
	.tag_filter = ""
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
static void *flush_worker(void *arg);
static void update_write_stat(size_t write_len);
static int mslog_mmap_init(void);
static void mslog_mmap_destroy(void);
static int mslog_mmap_write(const char *buf, size_t len);
static mslog_buf_node_t *mem_pool_alloc(void);
static void mem_pool_free(mslog_buf_node_t *node);
static int mem_pool_init(void);
static void mem_pool_destroy(void);

//-----------write count-----------//
static void update_write_stat(size_t write_len){
	time_t now = time(NULL);
	mslog_lock(MSLOG_INFO);
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
	mslog_unlock(MSLOG_INFO);
}

//------------mmap---------------//
static int mslog_mmap_init(void){
	if ( g_mslog.log_fp == NULL ){
		return -1;
	}

	int fd = fileno(g_mslog.log_fp);
	off_t file_size = lseek(fd, 0, SEEK_END);
	if ( file_size < 0 ){
		return -1;
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

//---------------mem pool--------------//
static int mem_pool_init(void){
	pthread_mutex_lock(&g_mslog.log_buf_pool.pool_mutex);

	for ( size_t i = 0; i < g_mslog.log_buf_pool.pool_size; i++ ){
		mslog_buf_node_t *node = (mslog_buf_node_t *)malloc(sizeof(mslog_buf_node_t));
		if ( node == NULL ){
			pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
			return -1;
		}
		node->buf = (char *)malloc(g_mslog.log_buf_pool.buf_size);
		if ( node-> buf == NULL ){
			free(node);
			pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
			return -1;
		}
		node->size = g_mslog.log_buf_pool.buf_size;
		node->next = g_mslog.log_buf_pool.free_list;
		g_mslog.log_buf_pool.free_list = node;
	}
	pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
	return 0;
}

static void mem_pool_destroy(void){
	pthread_mutex_lock(&g_mslog.log_buf_pool.pool_mutex);
	mslog_buf_node_t *curr = g_mslog.log_buf_pool.free_list;
	while ( curr != NULL ){
		mslog_buf_node_t *next =curr->next;
		free(curr->buf);
		free(curr);
		curr = next;
	}
	g_mslog.log_buf_pool.free_list = NULL;
	pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
}

static mslog_buf_node_t *mem_pool_alloc(void){
	pthread_mutex_lock(&g_mslog.log_buf_pool.pool_mutex);
	mslog_buf_node_t *node = g_mslog.log_buf_pool.free_list;
	if ( node != NULL ){
		g_mslog.log_buf_pool.free_list = node->next;
	}
	pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
	return node;
}

static void mem_pool_free(mslog_buf_node_t *node){
	if ( node == NULL ){
		return;
	}
	pthread_mutex_lock(&g_mslog.log_buf_pool.pool_mutex);
	node->next = g_mslog.log_buf_pool.free_list;
	g_mslog.log_buf_pool.free_list = node;
	pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
}

//----------------tool func tag match-----------------//
static int tag_match(const char *tag){
	if ( g_mslog.log_path[0] == '\0' || tag == NULL ){
		return 1;
	}

	char filter_copy[1024];
	strncpy(filter_copy, g_mslog.tag_filter, ( sizeof(filter_copy) - 1 ));
	filter_copy[sizeof(filter_copy) - 1] = '\0';

	char *token = strtok(filter_copy, "|");
	while( token != NULL ){
		if ( strcmp(token, tag) == 0 ){
			return 1;
		}
		token = strtok(NULL, "|");
	}
	return 0;
}

//--------------------tool func time cache refresh--------------------//
static void update_time_cache(void){
	time_t now = time(NULL);
	if ( now != g_mslog.last_time ){
		struct tm *tm =localtime(&now);
		strftime(g_mslog.time_cache, sizeof(g_mslog.time_cache), "%Y-%m-%d %H:%M:$S", tm);
		g_mslog.last_time = now;
	}
}

//--------------------tool func log rotate---------------------//
static void log_rotate(void){
	if ( g_mslog.log_fp == NULL || g_mslog.rotate_size <= 0 || g_mslog.rotate_num <= 0 ){
		return;
	}

	fflush(g_mslog.log_fp);
	struct stat st;

	if ( st.st_size < 0 ){
		return;
	}

	if ( fstat(fileno(g_mslog.log_fp), &st) < 0 || ( (size_t)st.st_size < g_mslog.rotate_size ) ){
		return;
	}

	char old_path[256];
	char new_path[256];
	const size_t max_path_len = sizeof(old_path) - 12;
	size_t log_path_len = strlen(g_mslog.log_path);
	if ( log_path_len >= max_path_len ){
		return;
	}

	mslog_mmap_destroy();

	for ( int i = g_mslog.rotate_num -1; i > 0; i-- ){
		snprintf(old_path, sizeof(old_path), "%s.%d", g_mslog.log_path, i);
		snprintf(new_path, sizeof(new_path), "%s.%d", g_mslog.log_path, i + 1);
		rename(old_path, new_path);
	}

	snprintf(old_path, sizeof(old_path), "%s.1", g_mslog.log_path);
	fclose(g_mslog.log_fp);
	rename(g_mslog.log_path, old_path);

	g_mslog.log_fp = fopen(g_mslog.log_path, "a");
	if ( g_mslog.log_fp == NULL )
		return;

	mslog_mmap_init();
}

//------------------tool func do flush---------------//
static void do_flush(void){
	if ( g_mslog.log_fp == NULL || g_mslog.curr_buf_used == 0 || g_mslog.io_buf == NULL ){
		return;
	}

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	mslog_lock(MSLOG_INFO);
	if ( mslog_mmap_write(g_mslog.io_buf, g_mslog.curr_buf_used) != 0 ){
		fwrite(g_mslog.io_buf, 1, g_mslog.curr_buf_used, g_mslog.log_fp);
	}
	fflush(g_mslog.log_fp);
	g_mslog.curr_buf_used = 0;
	mslog_unlock(MSLOG_INFO);

	clock_gettime(CLOCK_MONOTONIC, &end);
	size_t flush_time = ( end.tv_sec - start.tv_sec ) * 1000000 + ( end.tv_nsec - start.tv_nsec ) / 1000;
	g_total_flush_time += flush_time;
	g_flush_count++;
}

//---------------------tool func flush thread-----------------------//
static void *flush_worker(void *arg){
	(void)arg;
	while ( g_mslog.flush_running ){
		sleep(MSLOG_FLUSH_INTERVAL);
		do_flush();

		g_mslog.rotate_check_cnt++;
		if ( g_mslog.rotate_check_cnt >= MSLOG_ROTATE_CHECK_CNT_MAX ){
			log_rotate();
			g_mslog.rotate_check_cnt = 0;
		}
	}
	do_flush();
	return NULL;
}

//---------------------output interface init--------------------------//
int mslog_init(const char *log_path, mslog_level_t log_level, size_t rotate_size, int rotate_num, mslog_flush_mode_t flush_mode){
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

	g_mslog.io_buf = (char *)malloc(g_mslog.max_batch_threshold);
	if ( g_mslog.io_buf == NULL ){
		goto err_mutex;
	}

	if ( mem_pool_init() != 0 ){
		goto err_io_buf;
	}

	g_mslog.log_fp = fopen(g_mslog.log_path, "a");
	if ( g_mslog.log_fp == NULL ){
		goto err_mem_pool;
	}

	mslog_mmap_init();

	if ( g_mslog.flush_mode == MSLOG_FLUSH_BATCH ){
		g_mslog.flush_running = 1;
		if ( pthread_create(&g_mslog.flush_thread, NULL, flush_worker, NULL) != 0 ){
			g_mslog.flush_running = 0;
			goto err_log_fp;
		}
	}

	update_time_cache();
	return 0;

err_log_fp:
	fclose(g_mslog.log_fp);
	g_mslog.log_fp = NULL;
	mslog_mmap_destroy();

err_mem_pool:
	mem_pool_destroy();

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
	g_mslog.flush_running = 0;
	if ( g_mslog.flush_thread != 0 ){
		pthread_join(g_mslog.flush_thread, NULL);
	}

	do_flush();

	mslog_mmap_destroy();

	if ( g_mslog.log_fp != NULL ){
		fclose(g_mslog.log_fp);
		g_mslog.log_fp = NULL;
	}

	mem_pool_destroy();

	if ( g_mslog.io_buf != NULL ){
		free(g_mslog.io_buf);
		g_mslog.io_buf = NULL;
	}

	for ( int i = 0; i < MSLOG_LEVEL_MAX; i++ ){
		pthread_mutex_destroy(&g_mslog.level_mutex[i]);
	}
}

//-------------------output interface log write----------------------//
void mslog_log(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...){
	if ( level < 0 || level >= MSLOG_LEVEL_MAX || level < g_mslog.log_level ){
		return;
	}

	if ( !tag_match(tag) ){
		return;
	}

	mslog_buf_node_t *log_node = mem_pool_alloc();
	if ( log_node == NULL ){
		return;
	}
	char *log_buf = log_node->buf;
	size_t buf_size = log_node->size;
	size_t len = 0;

	const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

	update_time_cache();
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
	if ( g_mslog.flush_mode == MSLOG_FLUSH_REALTIME ){
		if ( mslog_mmap_write(log_buf, len) != 0 ){
			fwrite(log_buf, 1, len, g_mslog.log_fp);
		}
		fflush(g_mslog.log_fp);
		update_write_stat(len);
	}
	else
	{
		if ( g_mslog.curr_buf_used + len > g_mslog.batch_threshold ){
			do_flush();
		}
		if ( g_mslog.curr_buf_used + len <= g_mslog.max_batch_threshold ){
			memcpy(g_mslog.io_buf + g_mslog.curr_buf_used, log_buf, len);
			g_mslog.curr_buf_used += len;
			update_write_stat(len);
		}

		if ( (double)( g_mslog.curr_buf_used / g_mslog.max_batch_threshold > 0.8 ) ){
			do_flush();
		}
	}
	mslog_unlock(level);

	mem_pool_free(log_node);

	g_mslog.rotate_check_cnt++;
	if ( g_mslog.rotate_check_cnt >= MSLOG_ROTATE_CHECK_CNT_MAX ){
		log_rotate();
		g_mslog.rotate_check_cnt = 0;
	}
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

//-----------------------mem pool------------------//
/*
static int mem_pool_init(void){
	return g_mslog.log_buf_pool.pool_size > 0 ? 0 : -1;
}

static void mem_pool_destroy(void){
	mslog_buf_node_t *curr = g_mslog.log_buf_pool.free_list;
	while ( curr != NULL ){
		mslog_buf_node_t *next = curr->next;
		free(curr->buf);
		free(curr);
		curr = next;
	}
	g_mslog.log_buf_pool.free_list = NULL;
}

static mslog_buf_node_t *mem_pool_alloc(void){
	pthread_mutex_lock(&g_mslog.log_buf_pool.pool_mutex);
	mslog_buf_node_t *node = g_mslog.log_buf_pool.free_list;
	if ( node != NULL ){
		g_mslog.log_buf_pool.free_list = node->next;
	}
	pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
	return node;
}

static void mem_pool_free(mslog_buf_node_t *node){
	if ( node == NULL ){
		return;
	}
	pthread_mutex_lock(&g_mslog.log_buf_pool.pool_mutex);
	node->next = g_mslog.log_buf_pool.free_list;
	g_mslog.log_buf_pool.free_list = node;
	pthread_mutex_unlock(&g_mslog.log_buf_pool.pool_mutex);
}
*/
