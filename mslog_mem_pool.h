#pragma once
#ifndef __MSLOG_MEM_POOL_H__
#define __MSLOG_MEM_POOL_H__

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MSLOG_MEM_POOL_BLOCK_SIZE   ( 4096 )
#define MSLOG_MEM_POOL_MAX_BLOCKS   ( 1024 )

typedef struct{
    char *pool_buf;
    size_t pool_size;
    size_t used_size;
    pthread_mutex_t pool_mutex;
}mslog_mem_pool_t;

extern mslog_mem_pool_t g_mslog_mem_pool;

int mslog_mem_pool_init(void);
void *mslog_mem_alloc(size_t size);
void mslog_mem_free(void *ptr);
void mslog_mem_pool_deinit(void);

#endif//__MSLOG_MEM_POOL_H__
