#define _POSIX_C_SOURCE 200112L
#include "mslog.h"

typedef struct{
	mslog_level_t log_level;
	char tag_filter[1024];//tag filter
	char log_path[MSLOG_MAX_PATH_LEN];//log path
	size_t rotate_size;//rotate file size(bit)
	int rotate_num;//rotate file num
	mslog_flush_mode_t flush_mode;//flush mode

	pthread_mutex_t lock_bucket[MSLOG_LEVEL_MAX];//bucket lock by level
	char *io_buf;//io buf
	size_t io_buf_len;//io buf len
	pthread_t flush_thread;//auto flush thread
	int flush_running;//flush thread running flag
	char time_cache[32];//timestamp cache
	pthread_mutex_t time_lock;//time cache lock
	time_t last_time;//last update time
	int rotate_check_cnt;//rotate check counter
	
	FILE *log_fp;//log file handle
}mslog_global_t;

static mslog_global_t g_mslog = {
	.log_level = MSLOG_DEBUG,
	.rotate_size = MSLOG_DEFAULT_ROTATE_SIZE,
	.rotate_num = MSLOG_DEFAULT_ROTATE_NUM,
	.flush_mode = MSLOG_FLUSH_BATCH,
	.io_buf_len = 0,
	.flush_running = 0,
	.last_time = 0,
	.rotate_check_cnt = 0,
	.log_fp = NULL
};

static void update_time_cache(void){
	time_t now = time(NULL);
	pthread_mutex_lock(&g_mslog.time_lock);
	if ( now - g_mslog.last_time >= 1 ){
		struct tm *tm = localtime(&now);
		strftime(g_mslog.time_cache, sizeof(g_mslog.time_cache),
				"%Y-%m-%d %H:%M%S", tm);
		g_mslog.last_time = now;
	}
	pthread_mutex_unlock(&g_mslog.time_lock);
}

static int tag_match(const char *tag){
        if ( g_mslog.tag_filter[0] == '\0' || tag == NULL )
		return 1;

	char filter_copy[1024];
	strncpy(filter_copy, (const char *)g_mslog.tag_filter, ( sizeof(filter_copy) - 1 ));
	filter_copy[sizeof(filter_copy) - 1] = '\0';

	char *token = strtok(filter_copy, "|");
	while ( token != NULL ){
		if ( strcmp(token, tag) == 0 )
			return 1;
		token = strtok(NULL, "|");
	}
        return 0;
}

//reduce use stat times
static void mslog_rotate(void){
	g_mslog.rotate_check_cnt = 0;//refresh timer
	
	struct stat st;
	if ( stat(g_mslog.log_path, &st) < 0 )
		return;
	if ( st.st_size < 0 )
		return;

	size_t file_size = (size_t)st.st_size;
	if (file_size < g_mslog.rotate_size )
		return;

	//close this file
	if ( g_mslog.log_fp != NULL ){
		fclose(g_mslog.log_fp);
		g_mslog.log_fp = NULL;
	}

	//rotate file
	char old_path[MSLOG_MAX_PATH_LEN];
	char new_path[MSLOG_MAX_PATH_LEN];
	const size_t max_path_len = sizeof(old_path) - 12;

	size_t log_path_len = strlen(g_mslog.log_path);
	if ( log_path_len >= max_path_len ){
		fprintf(stderr, "log path is too long, can't generate rotate file path");
		return;
	}

	for ( int i = g_mslog.rotate_num; i > 0; i-- ){
		snprintf(old_path, sizeof(old_path), "%s.%d", g_mslog.log_path, i);
		snprintf(new_path, sizeof(new_path), "%s.%d", g_mslog.log_path, i+1);
		if ( access(old_path, F_OK) == 0 ){
			rename(old_path, new_path);
		}
	}

	//this file
	snprintf(old_path, sizeof(old_path), "%s.1", g_mslog.log_path);
	rename(g_mslog.log_path, old_path);

	//open file again
	g_mslog.log_fp = fopen(g_mslog.log_path, "a+");
	if ( g_mslog.log_fp == NULL ){
		perror("fopen failed");
	}
}

//auto do flush func
static void do_flush(void){
	if ( g_mslog.log_fp == NULL || g_mslog.io_buf_len == 0 )
		return;

	pthread_mutex_lock(&g_mslog.lock_bucket[0]);//global flushing lock
	size_t write_len = fwrite(g_mslog.io_buf, 1, g_mslog.io_buf_len, g_mslog.log_fp);
	if ( write_len > 0 ){
		g_mslog.io_buf_len = 0;//refresh buf
		fflush(g_mslog.log_fp);
	}
	pthread_mutex_unlock(&g_mslog.lock_bucket[0]);
}

//auto do flush thread
static void *flush_worker(void *arg){
	(void)arg;
	while ( g_mslog.flush_running ){
		sleep(MSLOG_FLUSH_INTERVAL);
		do_flush();
	}
	return NULL;
}

//bucket lock lock/unlock
static inline void lock(mslog_level_t level){
	if ( level >= MSLOG_LEVEL_MAX )
		level = MSLOG_DEBUG;
	pthread_mutex_lock(&g_mslog.lock_bucket[level]);
}

static inline void unlock(mslog_level_t level){
	if ( level >= MSLOG_LEVEL_MAX )
		level = MSLOG_DEBUG;
	pthread_mutex_unlock(&g_mslog.lock_bucket[level]);
}

//-------------------------------interface--------------------------------//
//get log level(no lock)
mslog_level_t mslog_get_level(void){
        mslog_level_t level = g_mslog.log_level;
        return level;
}

//global config interface init
int mslog_init(const char *log_path, size_t rotate_size, int rotate_num){
	//init bucket lock
	for ( int i = 0; i < MSLOG_LEVEL_MAX; i++ ){
		pthread_mutex_init(&g_mslog.lock_bucket[i], NULL);
	}
	pthread_mutex_init(&g_mslog.time_lock, NULL);

	//setting log file path
	if ( log_path != NULL && strlen(log_path) > 0 ){
		strncpy(g_mslog.log_path, log_path, ( sizeof(g_mslog.log_path) - 1 ));
	}
	else{
		strcpy(g_mslog.log_path, "./mslog.log");
	}

	//setting rotate num & size
	if ( rotate_size > 0 )
		g_mslog.rotate_size = rotate_size;
	if ( rotate_num > 0 )
		g_mslog.rotate_num = rotate_num;

	//setting io buf
	g_mslog.io_buf = (char *)malloc(MSLOG_IO_BUF_SIZE);
	if ( g_mslog.io_buf == NULL ){
		perror("malloc io buf failed");
		return -1;
	}
	g_mslog.io_buf = 0;

	//open log file
	g_mslog.log_fp = fopen(g_mslog.log_path, "a+");
	if ( g_mslog.log_fp == NULL ){
		perror("fopen log file failed");
		free(g_mslog.log_fp);
		return -1;
	}

	//start auto flush thread
	g_mslog.flush_running = 1;
	if ( pthread_create(&g_mslog.flush_thread, NULL, flush_worker, NULL) != 0 ){
		perror("pthread_create flush_thread failed");
		fclose(g_mslog.log_fp);
		free(g_mslog.io_buf);
		g_mslog.flush_running = 0;
		return -1;
	}

	//init time cache
	update_time_cache();
	
	return 0;
}

//global config interface deinit
void mslog_deinit(void){
	//stop auto flush thread
	g_mslog.flush_running = 0;
	pthread_join(g_mslog.flush_thread, NULL);
	
	do_flush();
	if ( g_mslog.log_fp != NULL ){
		fclose(g_mslog.log_fp);
		g_mslog.log_fp = NULL;
	}
	//release source
	free(g_mslog.io_buf);
	g_mslog.io_buf = NULL;

	//destroy lock
	for ( int i = 0; i < MSLOG_LEVEL_MAX; i++ ){
		pthread_mutex_destroy(&g_mslog.lock_bucket[i]);
	}
	pthread_mutex_destroy(&g_mslog.time_lock);

	//reset config
	memset(&g_mslog, 0, sizeof(g_mslog));
}

//global config interface setting log level
void mslog_set_level(mslog_level_t level){
	if ( level >= MSLOG_LEVEL_MAX )
		return;
	lock(MSLOG_DEBUG);//use debug level modify
	g_mslog.log_level = level;
	unlock(MSLOG_DEBUG);
}

void mslog_set_flush_mode(mslog_flush_mode_t mode){
	lock(MSLOG_DEBUG);
	g_mslog.flush_mode = mode;
	unlock(MSLOG_DEBUG);
}

void mslog_set_tag_filter(const char *tag_filter){
	if ( tag_filter == NULL )
		return;
	lock(MSLOG_DEBUG);
	strncpy(g_mslog.tag_filter, tag_filter, ( sizeof(g_mslog.tag_filter) - 1 ));
	unlock(MSLOG_DEBUG);
}

void mslog_flush(void){
	do_flush();
}

void mslog_core(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...){
	if ( !tag_match(tag) )
		return;
	
	lock(level);

	char log_buf[MSLOG_MAX_LOG_LEN];
	size_t len = 0;

	update_time_cache();
	const char *level_str[] = {"DEBUG","INFO","WARN","ERROR","FATAL"};

	//format: [time] [level] [tag] [file:line:func] msg
	len += snprintf(log_buf + len, ( sizeof(log_buf) - len ),
				"[%s][%s][%s] %s:%d:%s ",
				g_mslog.time_cache,
				level_str[level],
				tag ? tag : NULL,
				file, line, func );

	va_list ap;
	va_start(ap, fmt);
	len += vsnprintf(log_buf + len, ( sizeof(log_buf) - len ), fmt, ap);
	va_end(ap);

	if ( len < ( sizeof(log_buf) - 1 ) ){
		log_buf[len++] = '\n';
		log_buf[len] = '\0';
	}

	g_mslog.rotate_check_cnt++;
	if ( g_mslog.rotate_check_cnt >= MSLOG_ROTATE_CHECK_CNT ){
		mslog_rotate();
	}

	if ( g_mslog.flush_mode == MSLOG_FLUSH_REALTIME ){
		if ( g_mslog.log_fp != NULL ){
			fwrite(log_buf, 1, len, g_mslog.log_fp);
			fflush(g_mslog.log_fp);
		}
		else{
			if ( g_mslog.io_buf_len + len >= MSLOG_IO_BUF_SIZE ){
				do_flush();
			}
			memcpy(g_mslog.io_buf + g_mslog.io_buf_len, log_buf, len);
			g_mslog.io_buf_len += len;
		}
	}
	unlock(level);
}
