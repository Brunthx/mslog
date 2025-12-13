#define _POSIX_C_SOURCE 200112L
#include "mslog.h"

typedef struct{
	mslog_level_t log_level;
	char tag_filter[1024];//tag filter
	int enable_line_info;
	int enable_stdout;
	char log_path[MSLOG_MAX_PATH_LEN];//log path
	size_t rotate_size;//rotate file size(bit)
	int rotate_num;//rotate file num
	pthread_mutex_t mutex;// thread mutex
}mslog_global_t;

static mslog_global_t g_mslog = {0};
static const char *g_level_str[MSLOG_LEVEL_MAX] = { "Debug", "Info", "Warn", "Error", "Fatal" };
static const char *g_level_color[MSLOG_LEVEL_MAX] = {
	MS_COLOR_DEBUG, MS_COLOR_INFO, MS_COLOR_WARN, MS_COLOR_ERROR, MS_COLOR_FATAL
};

static char *ms_strdup(const char *s){
	if ( s == NULL ){
		return NULL;
	}

	size_t len = strlen(s) + 1;// +1 include '\0'
	char *new_str = (char*)malloc(len);
	if ( new_str != NULL ){
		memcpy(new_str, s, len);
	}
	return new_str;
}

static char *mslog_get_time(void){
	static char time_buf[MSLOG_MAX_TIME_LEN];
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H-%M:%S", tm);
	return time_buf;
}

static int mslog_rotate(void){
	struct stat st;
	if ( stat(g_mslog.log_path, &st) < 0 ){
		perror("mslog_rotate: state log file failed");
		return -1;
	}

	size_t file_size = (size_t)st.st_size;
	if( file_size < g_mslog.rotate_size ){
		return 0;
	}

	char old_path[MSLOG_MAX_PATH_LEN];
	char new_path[MSLOG_MAX_PATH_LEN];

	size_t base_len = strlen(g_mslog.log_path);
	if ( base_len + 16 > MSLOG_MAX_PATH_LEN ){
		fprintf(stderr,"mslog_rotate: log path too long: %s\n",g_mslog.log_path);
		return -1;
	}

	for ( int i = g_mslog.rotate_num; i > 0; i-- ){
		snprintf( old_path, sizeof(old_path), "%s.%d", g_mslog.log_path, i );
		snprintf( new_path, sizeof(new_path), "%s.%d", g_mslog.log_path, i+1 );
		if ( access( old_path, F_OK ) == 0 ){
			rename( old_path, new_path );
		}
	}

	//rename log name: log.log.1
	snprintf( old_path, sizeof(old_path), "%s.1", g_mslog.log_path );
	rename( g_mslog.log_path, old_path);

	//only write 644
	FILE *fp = fopen(g_mslog.log_path, "w");
	if ( fp != NULL){
		fclose(fp);
	}
	else
	{
		perror("mslog_rotate: create new log file failed");
		return -1;
	}
	return 0;
}

static int mslog_tag_match(const char *tag){
	if ( tag == NULL || g_mslog.tag_filter[0] == '\0'){
		return 1;// tag is NULL or no tag just pass
	}

	//multi tag match use | to separator
	char *filter = ms_strdup(g_mslog.tag_filter);
	if ( filter == NULL )
		return 1;
	char *token = strtok(filter, "|");
	int match = 0;
	while ( token != NULL ){
		if ( strcmp(token, tag) == 0 ){
			match = 1;
			break;
		}
		token = strtok (NULL, "|");
	}
	free(filter);
	return match;
}

//global config interface init
void mslog_init(const char *log_path, size_t rotate_size, int rotate_num){
	g_mslog.log_level = MSLOG_DEBUG;
	g_mslog.enable_line_info = 1;
	g_mslog.enable_stdout = 1;
	g_mslog.rotate_size = (rotate_size == 0) ? MSLOG_DEFAULT_ROTATE_SIZE : rotate_size;
	g_mslog.rotate_num = (rotate_num == 0) ? MSLOG_DEFAULT_ROTATE_NUM : rotate_num;
	memset(g_mslog.tag_filter, 0, sizeof(g_mslog.tag_filter));
	
	if ( log_path != NULL && strlen(log_path) > 0 ){
		strncpy(g_mslog.log_path, log_path, (sizeof(g_mslog.log_path) - 1));
	}
	else{
		strcpy(g_mslog.log_path, "./mslog.log");
	}

	//pthread_mutexattr_t attr;
	//pthread_mutexattr_init(&attr);
	//pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	//pthread_mutex_init(&g_mslog.mutex, &attr);
	//pthread_mutexattr_destroy(&attr);
	
	pthread_mutex_init(&g_mslog.mutex, NULL);

	FILE *fp = fopen(g_mslog.log_path, "a");
	if ( fp == NULL ){
		perror("mslog init: open log file failed");
		exit(EXIT_FAILURE);
	}
	fclose(fp);
}

//global config interface deinit
void mslog_deinit(void){
	pthread_mutex_destroy(&g_mslog.mutex);
	memset(&g_mslog, 0, sizeof(g_mslog));
}

//global config interface setting log level
void mslog_set_level(mslog_level_t level){
	if ( level >= 0 && level < MSLOG_LEVEL_MAX ){
		pthread_mutex_lock(&g_mslog.mutex);
		g_mslog.log_level = level;
		pthread_mutex_unlock(&g_mslog.mutex);
	}
}

void mslog_set_tag_filter(const char *tag_filter){
	pthread_mutex_lock(&g_mslog.mutex);
	if ( tag_filter != NULL ){
		strncpy(g_mslog.tag_filter, tag_filter, (sizeof(&g_mslog.tag_filter) - 1));
	}
	else{
		memset(g_mslog.tag_filter, 0, sizeof(&g_mslog.tag_filter));
	}
	pthread_mutex_unlock(&g_mslog.mutex);
}

void mslog_enable_line_info(int enable){
	pthread_mutex_lock(&g_mslog.mutex);
	g_mslog.enable_line_info = ( enable ? 1 : 0 );
	pthread_mutex_unlock(&g_mslog.mutex);
}

void mslog_enable_stdout(int enable){
	pthread_mutex_lock(&g_mslog.mutex);
	g_mslog.enable_stdout = ( enable ? 1 : 0 );
	pthread_mutex_unlock(&g_mslog.mutex);
}

void mslog_core(mslog_level_t level, const char *tag, const char *file, int line, const char *func, const char *fmt, ...){
	if ( level < 0 || level >= MSLOG_LEVEL_MAX || fmt == NULL ){
		return;
	}

	pthread_mutex_lock(&g_mslog.mutex);

	if ( level < g_mslog.log_level || !mslog_tag_match(tag) ){
		pthread_mutex_unlock(&g_mslog.mutex);
		return;
	}

	char log_buf[MSLOG_MAX_LOG_LEN] = {0};
	va_list ap;
	va_start(ap, fmt);

	int len = 0;
	len += snprintf(log_buf + len, ( sizeof(log_buf) - len ), 
			"[%s][%s][%s]",
			mslog_get_time(), g_level_str[level], ( tag ? tag : "NULL" ));

	if ( g_mslog.enable_line_info ){
		len += snprintf(log_buf + len, ( sizeof(log_buf) - len ), 
				MSLOG_FILE_LINE_FMT, file, line, func);
	}
	len += vsnprintf(log_buf + len, ( sizeof(log_buf) - len ), fmt, ap);
	va_end(ap);

	//rotate check before write file
	mslog_rotate();

	FILE *fp = fopen(g_mslog.log_path, "a");
	if (fp != NULL ){
		fprintf(fp, "%s\n", log_buf);
		fflush(fp);//force refresh
		fclose(fp);
	}

	if ( g_mslog.enable_stdout ){
		printf("%s%s%s\n", g_level_color[level], log_buf, MS_COLOR_RESET);
		fflush(stdout);
	}

	pthread_mutex_unlock(&g_mslog.mutex);
}
