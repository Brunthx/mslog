#define _POSIX_C_SOURCE 200112L
#include "mslog_mem_pool.h"

mslog_mem_pool_t g_mslog_mem_pool = {
    .pool_buf = NULL,
    .pool_size = 0,
    .used_size = 0
};

int mslog_mem_pool_init(void){
    g_mslog_mem_pool.pool_size = MSLOG_MEM_POOL_BLOCK_SIZE * MSLOG_MEM_POOL_MAX_BLOCKS;
    g_mslog_mem_pool.pool_buf = (char *)malloc(g_mslog_mem_pool.pool_buf);
    if ( g_mslog_mem_pool.pool_buf == NULL )
    {
        return -1;
    }
    memset(g_mslog_mem_pool.pool_buf, 0, g_mslog_mem_pool.pool_size);
    pthread_mutex_init(&g_mslog_mem_pool.pool_mutex);
    return 0;
}

void *mslog_mem_pool_alloc(size_t size){
    if ( size == 0 || size > g_mslog_mem_pool.pool_size )
    {
        return malloc(size);
    }

    pthread_mutex_lock(&g_mslog_mem_pool.pool_mutex);
    void *ptr = NULL;
    if ( g_mslog_mem_pool.used_size + size <= g_mslog_mem_pool.pool_size)
    {
        ptr = g_mslog_mem_pool.pool_buf + g_mslog_mem_pool.used_size;
    }
    else
    {
        ptr = malloc(size);
    }
    pthread_mutex_unlock(&g_mslog_mem_pool.pool_mutex);
    return ptr;
}

void mslog_mem_free(void *ptr){
    if ( ptr == NULL )
    {
        return;
    }
    if ( ptr >= (void *)g_mslog_mem_pool.pool_buf && ptr < (void *)(g_mslog_mem_pool.pool_buf + g_mslog_mem_pool.pool_size) )
    {
        pthread_mutex_lock(&g_mslog_mem_pool.pool_mutex);
        memset(ptr, 0, 1);
        pthread_mutex_unlock(&g_mslog_mem_pool.pool_mutex);
        return;
    }
    free(ptr);
}

void mslog_mem_pool_deinit(void){
    pthread_mutex_lock(&g_mslog_mem_pool.pool_mutex);
    if ( g_mslog_mem_pool.pool_buf != NULL )
    {
        free(g_mslog_mem_pool.pool_buf);
        g_mslog_mem_pool.pool_buf = NULL;
    }
    pthread_mutex_unlock(&g_mslog_mem_pool.pool_mutex);
    pthread_mutex_destroy(&g_mslog_mem_pool.pool_mutex);
}