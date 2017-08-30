/**
 * Copyright (C), 2017, progrom.
 * File name: thread_pool.c
 * Author: progrom
 * Version: 0.01
 * Date: 20170804 
 * Description: 线程池
 */

#include "thread_pool.h"
#include <stdlib.h>
#include <unistd.h>

/**
 * Function: thread_pool_init
 * Description: 初始化一个线程池
 * Input:
 *		tp_p 所要初始化的线程池
 *		pool_size 线程池能容纳的最大线程数
 *		queue_size 任务队列能缓存的最大任务数
 * Ouput:
 * Return:
 */
void thread_pool_init
(thread_pool_t *tp_p, int pool_size, int queue_size)
{
	if(tp_p==NULL || pool_size<1 || queue_size<1) {
		return;
	}

	tp_p->pool_thread = malloc(sizeof(pthread_t) * pool_size);
	tp_p->pool_size = pool_size;
	tp_p->thread_num = 0;
	tp_p->running_cnt = 0;

	/* 因为循环队列采用的实现是预留一个空位以区分队满和队空状态 */
	/* 故 queue_size + 1 */
	tp_p->work_queue = malloc(sizeof(thread_task_t) * (queue_size+1));
	tp_p->queue_size = queue_size+1;
	tp_p->queue_front = 0;
	tp_p->queue_rear = 0;

	tp_p->status = 1;

	pthread_mutex_init(&tp_p->lock, NULL);
	pthread_mutex_init(&tp_p->newtasklock, NULL);
	pthread_cond_init(&tp_p->wakeup, NULL);
}


/**
 * Function: thread_task_init
 * Description: 初始化一个任务
 * Input:
 *		tt_p 所要初始化的任务
 *		func 任务所要执行的函数
 *		arg 函数的参数
 * Ouput:
 * Return:
 */
void thread_task_init
(thread_task_t *tt_p, void (*func)(void *), void *argv)
{
	if(tt_p==NULL || func==NULL) {
		return;
	}

	tt_p->func = func;
	tt_p->argv = argv;
}


/**
 * Function: thread_pool_shutdown
 * Description:
 *		关闭一个线程池
 *		处于关闭状态的线程池不再接受任务
 *		如果选择不立即关闭线程池(now=0)则会等待直到所有任务(包括队列里的任务)完成
 * Input:
 *		tp_p 所要关闭的线程池
 *		now 是否立即关闭：0 否; 非0 是
 * Ouput:
 * Return:
 */
void thread_pool_shutdown(thread_pool_t *tp_p, int now)
{
	if(tp_p == NULL) {
		return;
	}

	/* 设置线程池状态为关闭 */
	pthread_mutex_lock(&tp_p->lock);
	tp_p->status = 0;
	pthread_mutex_unlock(&tp_p->lock);
	pthread_cond_broadcast(&tp_p->wakeup);

	/* 立即关闭，取消所有线程 */
	if(now != 0) {
		for(int i=0; i<tp_p->thread_num; ++i) {
			pthread_cancel(tp_p->pool_thread[i]);
		}
	}

	/* 等待所有线程结束 */
	for(int i=0; i<tp_p->thread_num; ++i) {
		pthread_join(tp_p->pool_thread[i], NULL);
	}

	/* 释放动态分配的内存 */
	free(tp_p->pool_thread);
	free(tp_p->work_queue);

	/* 销毁锁 */
	pthread_cond_destroy(&tp_p->wakeup);
	pthread_mutex_destroy(&tp_p->newtasklock);
	pthread_mutex_destroy(&tp_p->lock);
}


/**
 * Function: thread_pool_execute
 * Description: 令一个线程池执行一个任务
 * Input:
 *		tp_p 所要执行任务的线程池
 *		tt_p 所要执行的任务
 * Ouput:
 * Return:
 *		0 成功
 *		-1 失败
 */
int thread_pool_execute(thread_pool_t *tp_p, thread_task_t *tt_p)
{
	if(tp_p == NULL || tt_p==NULL) {
		return -1;
	}

	/* 先锁lock再锁newtasklock */
	pthread_mutex_lock(&tp_p->lock);

	pthread_mutex_lock(&tp_p->newtasklock); // 如果还不允许添加新任务则阻塞	
	pthread_mutex_unlock(&tp_p->newtasklock);

	/* 线程池已处于关闭状态 */
	if(tp_p->status <= 0) {
		pthread_mutex_unlock(&tp_p->lock);
		return -1;
	}

	/* 如果所有已创建的线程都在忙碌且线程池还未满则创建一个新的线程 */
	if(tp_p->running_cnt>=tp_p->thread_num && tp_p->thread_num<tp_p->pool_size) {
		int thread_num = tp_p->thread_num++;
		pthread_mutex_unlock(&tp_p->lock);
		pthread_create(&tp_p->pool_thread[thread_num], NULL, thread_execute, tp_p);
		pthread_mutex_lock(&tp_p->lock);
	}

	/* 任务入队 */
	int nrear = tp_p->queue_rear+1; // nrear = 尾指针后移一格
	if(nrear >= tp_p->queue_size) nrear = 0;
	if(nrear == tp_p->queue_front) { // 队列已满
		pthread_mutex_unlock(&tp_p->lock);
		return -1;
	}
	tp_p->work_queue[tp_p->queue_rear] = *tt_p; // 入队
	tp_p->queue_rear = nrear;

	/* 存在空闲的睡眠线程，唤醒线程去执行任务 */
	if(tp_p->running_cnt < tp_p->thread_num) {
		
		/* 轮询直到任务真正被唤醒的线程从队列取出，这儿用轮询因为个人觉得等待时间比较短没必要用条件变量增加开销 */
		pthread_mutex_lock(&tp_p->newtasklock); // 还未确定任务是否被新线程从队列取出，暂时不允许添加新任务

		pthread_mutex_unlock(&tp_p->lock);
		pthread_cond_signal(&tp_p->wakeup); // 唤醒线程

		/* 读取这三个临界资源不必用互斥量保护不影响结果的正确性，此外三个变量均声明为volatile */
		while(tp_p->queue_front!=tp_p->queue_rear && tp_p->status!=0) {} // 轮询直到线程池关闭或任务被取出
		pthread_mutex_unlock(&tp_p->newtasklock); // 允许添加新任务了
	} else {
		pthread_mutex_unlock(&tp_p->lock);
	}

	return 0;
}


/**
 * Function: thread_execute
 * Description:
 *		static函数，仅在本文件可见
 *		启动pthread线程所调用的函数
 *		主要就是不停地从队列中取出任务执行
 * Input:
 *		arg 线程所属的线程池
 * Ouput:
 * Return: NULL
 */
static void *thread_execute(void *arg)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); // 设置线程为异步取消

	thread_pool_t *tp_p = arg;
	thread_task_t task;

	for(;;) {

		pthread_mutex_lock(&tp_p->lock);

		/* 线程池已关闭且队列中没有未执行的任务 */
		if(tp_p->status<=0 && tp_p->queue_rear==tp_p->queue_front) {
			pthread_mutex_unlock(&tp_p->lock);
			break;
		}

		/* 等待队列任务的到来 */
		while(tp_p->queue_rear == tp_p->queue_front) {
			pthread_cond_wait(&tp_p->wakeup, &tp_p->lock);

			/* 线程池处于关闭状态且队列没有未执行任务，准备终止线程 */
			if(tp_p->status<=0 && tp_p->queue_rear==tp_p->queue_front) {
				pthread_mutex_unlock(&tp_p->lock);
				goto EXIT;
			}
		}

		/* 从队首取出任务 */
		task = tp_p->work_queue[tp_p->queue_front++];
		if(tp_p->queue_front >= tp_p->queue_size) tp_p->queue_front = 0;

		/* 执行任务 */
		++tp_p->running_cnt; // 正在执行任务的线程数+1
		pthread_mutex_unlock(&tp_p->lock);

		task.func(task.argv); // 调用执行任务的函数

		pthread_mutex_lock(&tp_p->lock);
		--tp_p->running_cnt; // 正在执行任务的线程数-1
		pthread_mutex_unlock(&tp_p->lock);
	}

EXIT: // 准备终止线程

	return NULL;
}
