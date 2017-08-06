/**
 * Copyright (C), 2017, progrom.
 * File name: test.c
 * Author: progrom
 * Version: 0.01
 * Date: 20170806
 * Description: 简单地测试一下线程池
 */

#include "thread_pool.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/* the arg of a task */
typedef struct task_arg {
	int num; /* the number of the task */
	int sec; /* working time of the task */
} task_arg;

void func(void *arg)
{
	task_arg *ta = arg;
	printf("\tI am task #%d and I'm going to sleep for %d second.\n", ta->num, ta->sec);
	sleep(ta->sec); // working...
	printf("\tI am task #%d and I have finished.\n", ta->num);
}

int main()
{
	thread_pool_t pool;

	puts("init a thread pool with pool size of 3 and queue size of 2.\n");
	thread_pool_init(&pool, 3, 2);

	thread_task_t task;
	task_arg args[5];
	
	for(int i=0; i<5; ++i) {
		args[i].num = i;
		args[i].sec = 2;
		thread_task_init(&task, func, &args[i]);

		printf("execute #%d task\n", i);
		int ret = thread_pool_execute(&pool, &task);
		printf("success: %d\n", ret);
		printf("pool.thread_num: %d\n", pool.thread_num);
		printf("pool.running_cnt: %d\n", pool.running_cnt);
		printf("pool.queue_front: %d\n", pool.queue_front);
		printf("pool.queue_rear: %d\n", pool.queue_rear);
		putchar('\n');
	}
	
	thread_pool_shutdown(&pool, 0); // wait until all tasks are finished
	return 0;
}
