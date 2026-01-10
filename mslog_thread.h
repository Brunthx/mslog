#pragma once

#ifndef __MSLOG_THREAD_H__
#define __MSLOG_THREAD_H__

#include "common.h"

#define MSLOG_THREAD_DETACHED		( 1 )
#define MSLOG_THREAD_JOINABLE		( 0 )

int mslog_thread_create(pthread_t *tid, int detach_mode, void *(*func)(void*), void *arg);
int mslog_thread_join(pthread_t tid);
void mslog_thread_sleep_ms(int ms);

#endif//__MSLOG_THREAD_H__
