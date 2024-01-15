#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define DEFAULT_MINTHREAD 5
#define DEFAULT_MAXTHREAD 10
#define DEFAULT_QUEUE_CAPACITY 100

enum STATUS_CODE
{
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    ACCESS_INVAILD,
    UNKNO,
};
/* 本质是一个消费者*/
void *threadHander(void *arg)
{
    /* 强制类型转换*/
    threadpool_t *pool = (threadpool_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&(pool->mutexpool));
        while (pool->queueSize == 0)
        {
            /* 等待一个条件变量，生产者发送过来的*/
            pthread_cond_wait(&(pool->notEmpty), &(pool->mutexpool));
        }
        /* 意味着队列有任务*/
        task_k tmpTask = pool->taskQueue[pool->queueFront];
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        /* 任务数减1*/
        pool->queueSize--;


        pthread_mutex_unlock(&(pool->mutexpool));
        /* 费者发信息给生产者消*/
        pthread_cond_signal(&(pool->notFull));

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNums++;
        pthread_mutex_unlock(&(pool->mutexBusy));

        /* 执行钩子函数--回调函数*/
        tmpTask.worker_hander(tmpTask.arg);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNums--;
        pthread_mutex_unlock(&(pool->mutexBusy));
    }
    pthread_exit(NULL);
}

/*  线程下池初始化*/
int threadPoolInit(threadpool_t *pool, int minThread, int maxThread, int queueCapacity)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    do
    {
        /* 判断合法性*/
        if (minThread < 0 || maxThread < 0 || minThread >= maxThread)
        {
            minThread = DEFAULT_MINTHREAD;
            maxThread = DEFAULT_MAXTHREAD;
        }
        /* 更新线程池属性*/
        pool->minThreads = minThread;
        pool->maxThreads = maxThread;

        /* 判断合法性*/
        if (queueCapacity <= 0)
        {
            queueCapacity = DEFAULT_QUEUE_CAPACITY;
        }
        /* 更新*/
        pool->queueCapacity = queueCapacity;
        pool->taskQueue = (task_k *)malloc(sizeof(task_k) * pool->queueCapacity);
        if (pool->taskQueue == NULL)
        {
            perror("malloc error");
            break;
        }
        memset(pool->taskQueue, 0, sizeof(task_k) * pool->queueCapacity);
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->queueSize = 0;

        pool->threadId = (pthread_t *)malloc(sizeof(pthread_t) * pool->maxThreads);
        if (pool->threadId == NULL)
        {
            perror("malloc error");
            exit(-1);
        }
        memset(pool->threadId, 0, sizeof(pthread_t) * pool->maxThreads);

        int ret = 0;
        /* 创建线程*/
        for (int idx = 0; idx < pool->minThreads; idx++)
        {
            /* 如果ID号为0 ，可以用*/
            if (pool->threadId[idx] == 0)
            {
                ret = pthread_create(&(pool->threadId[idx]), NULL, threadHander, pool);
                if (ret != 0)
                {
                    perror("thread create error");
                    break;
                }
            }
        }
        if (ret != 0)
        {
            break;
        }

        /* 初始化锁资源*/
        pthread_mutex_init(&(pool->mutexpool), NULL);
        pthread_mutex_init(&(pool->mutexBusy), NULL);
        /* 初始化条件变量*/
        if (pthread_cond_init(&(pool->notEmpty), NULL) != 0 || pthread_cond_init(&(pool->notFull), NULL) != 0)
        {
            perror("thread cond error");
            break;
        }
       

        return ON_SUCCESS;

    } while (0);
    /* 文件执行到这个地方， 上面一定有失败*/

    /* 回收堆空间*/
    if (pool->taskQueue != NULL)
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }

    /* 回收线程资源*/
    for (int idx = 0; idx < pool->minThreads; idx++)
    {
        if (pool->threadId[idx] != 0)
        {
            pthread_join(pool->threadId, NULL);
        }
    }

    if (pool->threadId != NULL)
    {
        free(pool->threadId);
        pool->threadId = NULL;
    }
    /* 释放锁资源*/
    pthread_mutex_destroy(&(pool->mutexpool));
    pthread_mutex_destroy(&(pool->mutexBusy));

    /* 释放条件变量*/
    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));

    return UNKNO;
}

/* 线程池添加任务*/
int threadPoolAddTask(threadpool_t *pool, void *(worker_hander)(void *), void *arg)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    /* 加锁*/
    pthread_mutex_lock(&(pool->mutexpool));
    /* 队列满了*/
    while (pool->queueSize == pool->queueCapacity)
    {
        pthread_cond_wait(&(pool->notFull), &(pool->mutexpool));
    }
    /* 到这一定有位置可以放任务*/
    /* 将任务放到队尾*/
    pool->taskQueue[pool->queueRear].worker_hander = worker_hander;
    pool->taskQueue[pool->queueRear].arg = arg;
    /* 队尾向后移动*/
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    pthread_mutex_unlock(&(pool->mutexpool));
    /* 发信号*/
    pthread_cond_signal(&pool->notEmpty);

    return ON_SUCCESS;
}

/* 线程池销毁*/
int threadPoolDestroy(threadpool_t *pool);
