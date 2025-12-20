#include"mslog.h"

#define THREAD_NUM	( 10 )
#define LOG_CNT		( 10000 )

void *log_worker(void *arg){
	int tid = *(int *)arg;
	char tag[32];
	snprintf(tag, sizeof(tag),"THREAD_%d", tid);

	for ( int i = 0; i < LOG_CNT; i++ ){
		MSLOG_INFO(tag, "test log %d, pid=%d, tid=%ld",
				i, getpid(), pthread_self());
	}
	return NULL;
}

int main(void){
	if ( mslog_init("./test_mslog.log", 10*1024*1024, 5) != 0 ){
		fprintf(stderr, "mslog init failed\n");
		return -1;
	}

	mslog_set_level(MSLOG_INFO);

	pthread_t thread[THREAD_NUM];
	int tids[THREAD_NUM];
	for ( int i = 0; i < THREAD_NUM; i++ ){
		tids[i] = i;
		pthread_create(&thread[i], NULL, log_worker, &tids[i]);
	}

	for ( int i = 0; i < THREAD_NUM; i++ ){
		pthread_join(thread[i], NULL);
	}

	mslog_flush();
	mslog_deinit();

	printf("mslog test finish! total log: %d\n", THREAD_NUM * LOG_CNT);
	return 0;
}
