#include "mslog_utils.h"

int mslog_utils_is_file_exist(const char *file_path){
	if ( file_path == NULL )
	{
		return 0;
	}
	return (access(file_path, F_OK) == 0) ? 1 : 0;
}

int mslog_utils_get_file_size(const char *file_path){
	if ( file_path == NULL || !mslog_utils_is_file_exist(file_path) )
	{
		return -1;
	}
	struct stat st;
	if ( stat(file_path, &st) != 0 )
	{
		return -1;
	}
	return (int)st.st_size;
}

void mslog_utils_get_time_str(char *buf, int buf_len){
	if ( buf == NULL || buf_len <= 0 )
	{
		return;
	}

	time_t now = time(NULL);
	struct tm *tm_now = localtime(&now);
	strftime(buf, buf_len, "%Y-%m-%d %H:%M:%S", tm_now);
}

int mslog_utils_log_rotate(const char *base_path, int max_count){
	if ( base_path == NULL || max_count <= 0 )
	{
		return -1;
	}
	if ( !mslog_utils_is_file_exist(base_path) )
	{
		return 0;
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

	snprintf(dst_path, sizeof(dst_path), "%s.1", base_path);
	rename(base_path, dst_path);
	return 1;
}