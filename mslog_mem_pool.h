#ifndef __MSLOG_MEM_POOL_H__
#define __MSLOG_MEM_POOL_H__

#define _POSIX_C_SOURCE 200112L

#include <pthread.h>
#include <stddef.h>

typedef struct mslog_buf_node{
    char *buf;
    size_t size;
    int occupied;
}mslog_buf_node_t;

typedef struct{
    mslog_buf_node_t *nodes;
    size_t node_count;
    size_t node_buf_size;
    pthread_mutex_t pool_mutex;
}mslog_mem_pool_t;

int mslog_mem_pool_init(mslog_mem_pool_t *pool, size_t node_buf_size, size_t node_count);

mslog_buf_node_t *mslog_mem_pool_alloc(mslog_mem_pool_t *pool);

void mslog_mem_pool_free(mslog_mem_pool_t *pool, mslog_buf_node_t *node);

void mslog_mem_pool_destroy(mslog_mem_pool_t *pool);

#endif//__MSLOG_MEM_POOL_H__
