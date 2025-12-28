#pragma once

#ifndef __MSLOG_UTILS_H__
#define __MSLOG_UTILS_H__

#include <stddef.h>
#include <time.h>

typedef struct mslog_global mslog_global_t;

int mslog_utils_tag_match(const char *tag, const char *log_path);

void mslog_utils_update_time_cache(char *time_cache, time_t *last_time, size_t cache_size);

void mslog_utils_log_rotate(mslog_global_t *g_mslog);

#endif//__MSLOG_UTIL_H__
