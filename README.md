# thread_pool
实现一个简单的线程池。
>[http://progrom.cn/2017/08/06/thread-pool-implementation/](http://progrom.cn/2017/08/06/thread-pool-implementation/)

## 测试
编译链接
```bash
make
```
运行测试
```bash
make run
```

## 用法
### 定义线程池变量
```C
thread_pool_t pool;
```
### 初始化线程池变量
```C
thread_pool_init(thread_pool_t *pool, int pool_size, int queue_size);
```
### 定义任务变量
```C
thread_task_t task;
```
### 初始化任务变量
```C
thread_task_init(thread_task_t *task, void (*func)(void *), void *argv);
```
### 将任务放入线程池执行
```C
thread_pool_execute(thread_pool_t *pool, thread_task_t *task);
```
### 关闭线程池
```C
thread_pool_shutdown(thread_pool_t *pool, int now);
```
