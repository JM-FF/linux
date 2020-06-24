#pragma once
#include<deque>
#include<pthread.h>
#include<string>
#include<string.h>
#include<stdlib.h>

//使用c++98 语言规范实现线程池：面向对象思想每一个任务都是Task继承类的对象

namespace z1
{
    class Task
    {
    public:
        Task(void* arg = NULL, const std::string taskName = "")
            :arg_(arg),taskName_(taskName)
        {
        }
        virtual ~Task()         //虚析构函数可以保证在派发生多态时可以用父类指针释放派生类对象的资源（调用派生类构造函数）
        {
        }

        void setArg(void *arg)
        {
            arg_ = arg;
        }

        virtual int run() = 0;  //纯虚函数由用户自己继承实现

    protected:
        void*    arg_;
        std::string taskName_;
    };
    
    class ThreadPool
    {
    public:
        ThreadPool(int threadNum = 10);
        ~ThreadPool();

    public:
        size_t addTask(Task *task);     //添加任务
        void   stop();                  //停止线程池
        int    size();                  //线程池的大小
        Task*  take();                  //获取任务

    private:
        int    createThreads();  //创建线程池，定义为私有在对象构造时初始化
        static void* threadFunc(void* threadData); //线程池创建调用的函数

    private:
        ThreadPool& operator=(const ThreadPool&);  //不允许拷贝构造拷贝赋值
        ThreadPool(const ThreadPool&);

    private:
        volatile bool       isRunning_;     //线程池是否运行
        int                 threadsNum_;    //线程数
        pthread_t*          threads_;       //线程ID数组

        std::deque<Task*>   taskQueue_;     //任务队列用容器实现比较方便
        pthread_mutex_t     mutex_;         //互斥锁
        pthread_cond_t      condition_;      //条件变量
    };
}




















































