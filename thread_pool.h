/**
 * Copyright (C), 2017, progrom.
 * File name: thread_pool.h
 * Author: progrom
 * Version: 0.01
 * Date: 20170804 
 * Description: 线程池头文件
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

/* 线程池结构体 */
typedef struct thread_pool_t {
	pthread_t *pool_thread; /* 线程数组 */
	int pool_size; /* 线程池内线程最大数目(数组大小) */
	int thread_num; /* 当前线程数 */
	int running_cnt; /* 正在执行任务的线程数 */

	struct thread_task_t *work_queue; /* 缓存未被处理任务的循环队列 */
	int queue_size; /* 队列大小 */
	volatile int queue_front; /* 队列头指针 */
	volatile int queue_rear; /* 队列尾指针 */

	volatile int status; /* 线程池状态：0 关闭; 1 开启*/

	pthread_mutex_t lock; /* 保护线程池临界资源的锁 */
	pthread_mutex_t newtasklock; /* 是否允许添加新任务，作用于令旧任务执行优先级大于新任务添加 */
	pthread_cond_t wakeup; /* 令线程休眠等待任务或唤醒执行任务的条件变量 */
} thread_pool_t;


/* 任务结构体 */
typedef struct thread_task_t {
	void (*func)(void *); /* 任务所要执行的函数 */
	void *argv; /* 函数参数 */
} thread_task_t;


void thread_pool_init(thread_pool_t *, int, int);

void thread_task_init(thread_task_t *, void (*)(void *), void *);

void thread_pool_shutdown(thread_pool_t *, int);

int thread_pool_execute(thread_pool_t *, thread_task_t *);

static void *thread_execute(void *); 

#endif
