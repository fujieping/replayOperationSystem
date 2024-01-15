#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_

#include <pthread.h>

typedef struct task_k
{
    /* 钩子*/
    void *(*worker_hander)(void *arg);
    /* 参数*/
    void *arg;
}task_k;


/* 线程结构体*/
typedef struct threadpool_t
{
    /* 任务队列*/
    task_k * taskQueue;
    /* 任务队列容量*/
    int queueCapacity;
    /* 队列中的任务数*/
    int queueSize;
    /* 队头*/
    int queueFront;
    /* 队尾*/
    int queueRear;

    /* 线程池中的线程*/
    pthread_t *threadId;

    /* 最小的线程数*/
    int minThreads;
    /* 最大的线程数*/
    int maxThreads;

    /* 干活的线程数*/
    int busyThreadNums;
    /* 存活的线程数*/
    int liveThreadNums;

    /* 维护整个线程池的*/
    pthread_mutex_t mutexpool;
    /* 只维护干活的线程*/
    pthread_mutex_t mutexBusy;
    /* 条件变量：*/
    pthread_cond_t notEmpty;
    /* 条件变量:有空位*/
    pthread_cond_t notFull;

}threadpool_t;


/*  线程下池初始化*/
int threadPoolInit(threadpool_t *pool, int minThread, int maxThread, int queueCapacity);

/* 线程池添加任务*/
int threadPoolAddTask(threadpool_t *pool, void *(worker_hander)(void *), void *arg);

/* 线程池销毁*/
int threadPoolDestroy(threadpool_t *pool);

#endif