#include "mslog_thread.h"

int mslog_thread_create(pthread_t *tid, int detach_mode, void *(*func)(void*), void *arg){
    if ( tid == NULL || func == NULL )
    {
        return -1;
    }
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if ( detach_mode == MSLOG_THREAD_DETACHED )
    {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    else
    {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }
    
    
    int ret = pthread_create(tid, &attr, func, arg);
    pthread_attr_destroy(&attr);
    return ret;
}

int mslog_thread_join(pthread_t tid){
    return pthread_join(tid, NULL);
}

void mslog_thread_sleep_ms(int ms){
    if ( ms <= 0 )
    {
        return;
    }
    struct timespec ts = {0};
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = ( ms % 1000 ) * 1000000L;
    nanosleep(&ts, NULL);
}