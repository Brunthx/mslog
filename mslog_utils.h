#pragma once

#ifndef __MSLOG_UTILS_H__
#define __MSLOG_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

//log rotate tool func interface
int mslog_utils_get_file_size(const char *file_path);
int mslog_utils_log_rotate(const char *base_path, int max_count);
void mslog_utils_get_time_str(char *buf, int buf_len);
int mslog_utils_is_file_exist(const char *file_path);

#endif//__MSLOG_UTIL_H__