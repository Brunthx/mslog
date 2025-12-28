#include "mslog_utils.h"
#include "mslog.h"

int mslog_utils_tag_match(const char *tag, const char *log_path){
	if ( log_path == NULL || log_path[0] == '\0' || tag == NULL ){
		return 1;
	}

	char filter_copy[1024];
	strncpy(filter_copy, log_path, ( sizeof(filter_copy) - 1 ));
	filter_copy[sizeof(filter_copy) - 1] = '\0';

	char *token = strtok(filter_copy, "|");
	while ( token != NULL ){
		if ( strcmp(token, tag) == 0 ){
			return 1;
		}
		token = strtok(NULL, "|");
	}
	return 0;
}

void mslog_utils_update_time_cache(char *time_cache, time_t *last_time, size_t cache_size){
	if ( time_cache == NULL || last_time == NULL || cache_size < 32 ){
		return;
	}

	time_t now = time(NULL);//sec

	if ( now != *last_time ){
		struct tm tm_buf;
		struct tm *tm = localtime_r(&now, &tm_buf);
		if ( tm == NULL ){
			snprintf(time_cache, cache_size, "UnknownTime");
			return;
		}
	}

	strftime(time_cache, cache_size, "%Y-%m-%d %H:%M:%S", tm);
	*last_time = now;
}

void mslog_utils_log_rotate(mslog_global_t *g_mslog){
	if ( g_mslog == NULL || g_mslog->log_fp == NULL || g_mslog->rotate_size <= 0 || g_mslog->rotate_num <= 0 ){
		return;
	}

	fflush(g_mslog->log_fp);

	struct stat st;
	int fd = fileno(g_mslog->log_fp);
	if ( fd < 0 || fstat(fd, &st) < 0 ){
		return;
	}

	if ( (size_t)st.st_size < g_mslog->rotate_size ){
		return;
	}

	char old_path[256];
	char new_path[256];
	const size_t max_path_len = sizeof(old_path) - 12;
	size_t log_path_len = strlen(g_mslog->log_path);
	if ( log_path_len >= max_path_len ){
		return;
	}

	if ( g_mslog->mmap_buf != MAP_FAILED ){
		msync(g_mslog->mmap_buf, g_mslog->mmap_offset, MS_SYNC);
		munmap(g_mslog->mmap_buf, g_mslog->mmap_size);
		g_mslog->mmap_buf = MAP_FAILED;
		g_mslog->mmap_offset = 0;
	}

	for ( int i = g_mslog->rotate_num -1; i > 0; i-- ){
		snprintf(old_path, sizeof(old_path), "%s.%d", g_mslog->log_path, i);
		snprintf(new_path, sizeof(new_path), "%s.%d", g_mslog->log_path, i + 1);
		rename(old_path, new_path);
	}

	snprintf(old_path, sizeof(old_path), "%s.1", g_mslog->log_path);
	fclose(g_mslog->log_fp);
	rename(g_mslog->log_path, old_path);

	g_mslog->log_fp = fopen(g_mslog->log_path, "a");
	if ( g_mslog->log_fp == NULL ){
		return;
	}

	off_t new_file_size = lseek(fileno(g_mslog->log_fp), 0, SEEK_END);
	if ( new_file_size >= g_mslog->mmap_min_file_size ){
		g_mslog->mmap_buf = mmap(NULL, g_mslog->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(g_mslog->log_fp), 0);
		if ( g_mslog->mmap_buf != MAP_FAILED ){
			g_mslog->mmap_offset = new_file_size;
		}
	}
}
