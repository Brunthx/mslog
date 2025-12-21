#include"mslog.h"

#define TEST_THREAD_NUM		( 5 )
#define LOG_PER_THREAD		( 1000 )

void *test_log_worker(void *arg){
	long thread_id = (long)arg;
	char tag[32];
	snprintf(tag, sizeof(tag),"THREAD_%ld", thread_id);
	struct timespec ts = {0, 100 * 1000};

	for ( int i = 0; i < LOG_PER_THREAD; i++ ){
		MSLOG_INFO(tag, "test log msg,index:%d", i);
		nanosleep(&ts, NULL);
	}
	return NULL;
}

int main(void){
	if ( mslog_init("./test_mslog.log", MSLOG_INFO, 10*1024*1024, 5, MSLOG_FLUSH_BATCH) != 0 ){
		printf("mslog init failed\n");
		return -1;
	}
	printf("mslog init success, start test...\n");

	pthread_t thread[TEST_THREAD_NUM];
	for ( long i = 0; i < TEST_THREAD_NUM; i++ ){
		if ( pthread_create(&thread[i], NULL, test_log_worker, (void *)i) != 0 ){
			printf("create thread %ld failed\n", i);
			mslog_deinit();
			return -1;
		}
	}

	for ( long i = 0; i < TEST_THREAD_NUM; i++ ){
		pthread_join(thread[i], NULL);
	}

	sleep(2);

	size_t total_bytes = 0;
	size_t avg_flush_time = 0;
	size_t buf_usage = 0;
	mslog_get_stats(&total_bytes, &avg_flush_time, &buf_usage);

	printf("\n=========mslog performance stats========\n");
	printf("total write byte: %zu KB\n", total_bytes / 1024);
	printf("average flush time: %zu us\n", avg_flush_time);
	printf("current buffer usage: %zu %%\n", buf_usage);
	printf("=========================================\n");

	mslog_deinit();
	printf("mslog deinit success, test finish!\n");
	return 0;
}
