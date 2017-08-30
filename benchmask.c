#include "thread_pool.h"
#include <stdio.h>
#include <sys/time.h>

#define MAX_QUE_SIZE 100000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
struct timeval now;
struct timespec timeout;

void io_tskfunc(void *arg)
{
	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec + 1;
	timeout.tv_nsec = now.tv_usec * 1000;
	pthread_mutex_timedlock(&mutex, &timeout);
}

void cpu_tskfunc(void *arg)
{
	double f = 0;
	for(int i=0; i<1000; ++i) {
		for(int j=0; j<1000; ++j) {
			for(int k=0; k<333; ++k) {
				f+=i;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int thrnum = 1; // 默认线程数 1
	int cpu_tsknum = 0; // 默认计算任务数 0
	int io_tsknum = 0; // 默认IO任务数 0
	
	for(int i=1; i<argc; ++i) {
		if(argv[i][0]=='-' && argv[i][2]==0) {
			switch(argv[i][1]) {
			case 'n':
				sscanf(argv[++i], "%d", &thrnum);
				break;
			case 'u':
				sscanf(argv[++i], "%d", &cpu_tsknum);
				break;
			case 'b':
				sscanf(argv[++i], "%d", &io_tsknum);
				break;
			default:
				break;
			}
		}
	}
	
	pthread_mutex_lock(&mutex);

	thread_pool_t pool;
	thread_pool_init(&pool, thrnum, MAX_QUE_SIZE);

	thread_task_t cpu_tsk;
	thread_task_t io_tsk;
	thread_task_init(&cpu_tsk, cpu_tskfunc, NULL);
	thread_task_init(&io_tsk, io_tskfunc, NULL);

	for(int i=0; i<io_tsknum; ++i) {
		thread_pool_execute(&pool, &io_tsk);
	}
	for(int i=0; i<cpu_tsknum; ++i) {
		thread_pool_execute(&pool, &cpu_tsk);
	}

	thread_pool_shutdown(&pool, 0);

	return 0;
}
