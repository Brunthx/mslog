#include"mslog.h"

#define THREAD_NUM	10
#define LOG_NUM_PRE_THREAD	100

void *thread_log_test(void *arg){
	int thread_id = *(int*)arg;
	char tag[32];
	snprintf(tag, sizeof(tag), "Thread-%d",thread_id);

	for ( int i = 0; i < LOG_NUM_PRE_THREAD; i++ ){
		MSLOG_DBG(tag, "debug log: index = %d", i);
		MSLOG_INFO(tag, "info log: index = %d", i);
	       if ( i % 10 == 0 )
	       {
			MSLOG_WARN(tag, "warn log: index = %d (10x interval)", i);
			MSLOG_ERR(tag, "error log: index = %d (10x interval)", i);
	       }

	       if ( i == LOG_NUM_PRE_THREAD - 1)
	       {
		       MSLOG_FATAL(tag, "fatal log: thread exit (index = %d)", i);
	       }
	}
	free(arg);
	return NULL;
}

int main(void)
{
	mslog_init("./test_mslog.log", 5 * 1024 * 1024, 3);

	mslog_set_tag_filter("Thread-0|Thread-1|Thread-2");
	//mslog_set_level(MSLOG_INFO);
	//mslog_enable_line_info(0);
	//mslog_enable_stdout(0);
	
	MSLOG_DBG("Main","mslog init success, start single-thread test");
	MSLOG_INFO("Main","system version: v1.0.0");
	MSLOG_WARN("Main","disk uage > 80% (simulate warning)");
	MSLOG_ERR("Main","network timeout (simulate error)");
	MSLOG_FATAL("Main","system will restart after 10s (simulate fatal)");

	pthread_t tid[THREAD_NUM];
	for ( int i = 0; i < THREAD_NUM; i++ )
	{
		int *thread_id = malloc(sizeof(int));
		*thread_id = i;
		pthread_create(&tid[i], NULL, thread_log_test, thread_id);
	}

	for ( int i = 0; i < THREAD_NUM; i++ )
	{
		pthread_join(tid[i], NULL);
	}

	MSLOG_INFO("Main","test finish, deinit mslog");
	mslog_deinit();
	return 0;
}
