#include <pthread.h>
#include <sched.h>
#include <time.h>
#define _POSIX_C_SOURCE 200112L

#include "mslog_mem_pool.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

size_t i,j;

int mslog_mem_pool_init(mslog_mem_pool_t *pool, size_t node_buf_size, size_t node_count){
    if ( pool == NULL || node_buf_size == 0 || node_count == 0 ){
        return -1;
    }

    memset(pool, 0, sizeof(mslog_mem_pool_t));

    pool->nodes = (mslog_buf_node_t *)calloc(node_count, sizeof(mslog_buf_node_t));
    if ( pool->nodes == NULL ){
        return -2;
    }

    pool->node_count = node_count;
    pool->node_buf_size = node_buf_size;
    for ( i = 0; i < node_count; i++ ){
        pool->nodes[i].buf = (char *)malloc(node_buf_size);
        if ( pool->nodes[i].buf == NULL ){
            for ( j = 0; j < i; j++ ){
                free(pool->nodes[j].buf);
            }
            free(pool->nodes);
            pool->nodes = NULL;
            return -2;
        }
        pool->nodes[i].size = node_buf_size;
        pool->nodes[i].occupied = 0;//free, unused
    }

    int ret = pthread_mutex_init(&pool->pool_mutex, NULL);
    if ( ret != 0 ){
        for ( i = 0; i < node_count; i++ ){
            free(pool->nodes[i].buf);
        }
        free(pool->nodes);
        pool->nodes = NULL;
        return -3;
    }
    return 0;
}

mslog_buf_node_t *mslog_mem_pool_alloc(mslog_mem_pool_t *pool){
    if ( pool == NULL || pool->nodes == NULL || pool->node_count == 0 ){
        return NULL;
    }

    pthread_mutex_lock(&pool->pool_mutex);

    mslog_buf_node_t *free_node = NULL;
    for ( i = 0; i < pool->node_count; i++ ){
        if ( pool->nodes[i].occupied == 0){
            pool->nodes[i].occupied = 1;
            free_node = &pool->nodes[i];
            break;
        }
    }

    pthread_mutex_unlock(&pool->pool_mutex);
    
    return free_node;
}

void mslog_mem_pool_free(mslog_mem_pool_t *pool, mslog_buf_node_t *node){
    if ( pool == NULL || pool->nodes == NULL || node == NULL ){
        return;
    }

    pthread_mutex_lock(&pool->pool_mutex);

    node->occupied = 0;

    pthread_mutex_unlock(&pool->pool_mutex);
}

void mslog_mem_pool_destroy(mslog_mem_pool_t *pool){
    if ( pool == NULL || pool->nodes == NULL ){
        return;
    }

    pthread_mutex_lock(&pool->pool_mutex);

    for ( i = 0; i < pool->node_count; i++ ){
        if ( pool->nodes[i].buf != NULL ){
            free(pool->nodes[i].buf);
            pool->nodes[i].buf = NULL;
        }
    }

    free(pool->nodes);
    pool->nodes = NULL;
    pool->node_count = 0;
    pool->node_buf_size = 0;

    pthread_mutex_unlock(&pool->pool_mutex);
    pthread_mutex_destroy(&pool->pool_mutex);

    memset(pool, 0, sizeof(mslog_mem_pool_t));
}
