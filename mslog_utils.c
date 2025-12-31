#define _POSIX_C_SOURCE 200112L
#include "mslog_utils.h"

int mslog_utils_get_file_size(const char *file_path){
	if ( file_path == NULL )
	{
		return -1;
	}

	struct stat st;
	if ( stat(file_path, &st) != 0 )
	{
		return -1;
	}
	return st.st_size;
}

int mslog_utils_is_file_exist(const char *file_path){
	return (access(file_path, F_OK) == 0) ? 1 : 0;
}

void mslog_utils_get_time_str(char *buf, int buf_len){
	if ( buf == NULL || buf_len <= 0 )
	{
		return;
	}

	time_t now = time(NULL);
	struct tm *tm_now = localtime(&now);
	snprintf(buf, buf_len, "%04d%02d%02d%02d%02d%02d",
             tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday,
             tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
}

int mslog_utils_log_rotate(const char *base_path, int max_count){
	if ( base_path == NULL || max_count <= 0 )
	{
		return -1;
	}
	
	char src_path[512] = {0}, dst_path[512] = {0};

	for (int i = max_count; i > 0; i--)
	{
		snprintf(src_path, sizeof(src_path), "%s.%d", base_path, i - 1);
		snprintf(dst_path, sizeof(dst_path), "%s.%d", base_path, i);
		if ( mslog_utils_is_file_exist(src_path) )
		{
			rename(src_path, dst_path);
		}
	}
	rename(base_path, dst_path);
	return 0;
}