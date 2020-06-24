#include "threadpool.h"
#include<stdio.h>
#include<assert.h>
#include<stdlib.h>

namespace z1
{
    ThreadPool::ThreadPool(int threadNum)
    {
        isRunning_  = true;         //将线程池设置为运行状态
        threadsNum_ = threadNum;    //设置线程池的数量
        createThreads();            //创建线程池
    }
    ThreadPool::~ThreadPool()
    {
        stop();
        for(std::deque<Task*>::iterator it = taskQueue_.begin();it != taskQueue_.end();it++)
        {
            delete *it;
        }
        taskQueue_.clear();
    }
    size_t ThreadPool::addTask(Task *task)     //添加任务
    {
        pthread_mutex_lock(&mutex_);  //互斥访问任务队列
        taskQueue_.push_back(task);
        int size = taskQueue_.size();
        pthread_mutex_unlock(&mutex_);  //解锁
        pthread_cond_signal(&condition_); //唤醒阻塞在condition上的函数
        return size;
    }
    void   ThreadPool::stop()                  //停止线程池
    {
        if(!isRunning_)     //如果为假直接退出
        {
            return;
        }

        isRunning_ = false; //否则设置为假
        pthread_cond_broadcast(&condition_);   //唤醒所有阻塞在条件变量上

        for(int i = 0;i < threadsNum_;i++)
        {
            pthread_join(threads_[i],NULL);    //回收每个线程
        }

        free(threads_);         //释放threads_资源
        threads_ = NULL;

        pthread_mutex_destroy(&mutex_);  //销毁互斥锁
        pthread_cond_destroy(&condition_); //销毁条件变量

    }
    int    ThreadPool::size()                   //线程池的大小
    {
        pthread_mutex_lock(&mutex_);
        int size = taskQueue_.size();
        pthread_mutex_unlock(&mutex_);
        return size;
    }
    Task*  ThreadPool::take()                 //获取任务
    {
        Task* task = NULL;
        while(!task)
        {
            pthread_mutex_lock(&mutex_);
            while(taskQueue_.empty() && isRunning_)
            {
                pthread_cond_wait(&condition_, &mutex_);  //任务队列为空阻塞等待任务来
            }
            if(!isRunning_)
            {
                pthread_mutex_unlock(&mutex_);
                break;
            }
            else if(taskQueue_.empty())
            {
                pthread_mutex_unlock(&mutex_);
                continue;
            }

            assert(!taskQueue_.empty());
            task = taskQueue_.front();          //去任务
            taskQueue_.pop_front();             //出队
            pthread_mutex_unlock(&mutex_);
        }
        return task;
    }

    int    ThreadPool::createThreads()  //创建线程池，定义为私有在对象构造时初始化
    {
        pthread_mutex_init(&mutex_, NULL);  //互斥锁初始化
        pthread_cond_init(&condition_,NULL);//环境变量初始化
        threads_ = (pthread_t*)malloc(sizeof(pthread_t)*threadsNum_);
        for(int i = 0;i < threadsNum_;i++)
        {
            pthread_create(&threads_[i],NULL,threadFunc,this); //循环创建
        }
        return 0;
    }
    void* ThreadPool::threadFunc(void* arg) //线程池创建调用的函数
    {
        pthread_t tid = pthread_self();  //获取当前线程ID
        ThreadPool* pool = static_cast<ThreadPool*>(arg);  //取出参数
        while(pool->isRunning_)      //当前线程处于运行状态
        {
            Task* task = pool->take();   //揽活
            if(!task)         //如果为空，退出
            {
                printf("Thread %lu will exit\n",tid);
                break;
            }
            assert(task);
            task->run();
        }
        return 0;
    }
}
