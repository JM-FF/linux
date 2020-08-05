#ifndef __M_POOL_H__
#define __M_POOL_H__
#include <iostream>
#include <queue>
#include <thread>
#include <pthread.h>
using namespace std;

typedef void(*Handler_t)(int);

class ThreadTask{
    private:
        int _sockfd;  //数据
        Handler_t _handler; //数据的处理方式

    public:
        //传入数据和数据的处理方式
        void SetTask(int sockfd, Handler_t handler){
            _sockfd = sockfd;
            _handler = handler;
        }

        //处理数据
        void TaskRun(){
            _handler(_sockfd);
        }
};


class ThreadPool{
    private:
        int _maxThread;     //创建线程的最大数量
        int _maxQueue;  //任务队列的最大数量
        
        queue<ThreadTask> _queue;
        
        pthread_mutex_t _mutex;     //互斥锁
        pthread_cond_t _producer;   //生产者
        pthread_cond_t _consumer;  //消费者
   
        //线程入口函数
        void ThreadStart(){
            while(1){
                //加锁
                pthread_mutex_lock(&_mutex);

                //若队列中没有任务
                while(_queue.empty()){
                    //等待 + 解锁
                    pthread_cond_wait(&_consumer, &_mutex);
                }

                //出队 
                ThreadTask task = _queue.front();
                _queue.pop();

                //解锁
                pthread_mutex_unlock(&_mutex);

                //唤醒生产者
                pthread_cond_signal(&_producer);

                //开始处理任务
                task.TaskRun();
            }
        }

    public:
        //构造函数
        ThreadPool(int maxThread = 10, int maxQueue = 10):
            _maxThread(maxThread), _maxQueue(maxQueue){
               
                pthread_mutex_init(&_mutex, NULL);
                pthread_cond_init(&_producer, NULL);
                pthread_cond_init(&_consumer, NULL);
            }

        //析构函数
        ~ThreadPool(){
            pthread_mutex_destroy(&_mutex);
            pthread_cond_destroy(&_producer);
            pthread_cond_destroy(&_consumer);
        }

        
        //创建线程
        bool PoolInit(){
            for(int i = 0; i < _maxThread; i++) {
                //创建线程
                thread thr = thread(&ThreadPool::ThreadStart, this);
                
                //线程分离
                thr.detach();
            }           
            
            return true;
        }
        
        //添加任务
        bool AddTask(ThreadTask& task){
            //加锁
            pthread_mutex_lock(&_mutex);
            
            //若任务队列已满
            while(_queue.size() == _maxQueue){
                //等待 + 解锁
                pthread_cond_wait(&_producer, &_mutex);
            }

            //入队
            _queue.push(task);

            //解锁
            pthread_mutex_unlock(&_mutex);

            //唤醒消费者
            pthread_cond_signal(&_consumer);

            return true;
        }
};
#endif
