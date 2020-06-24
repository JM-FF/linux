#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "threadpool.h" 

class MyTask:public z1::Task
{
    public:
        MyTask(){}
        virtual int run()
        {
             printf("thread[%lu] : %s\n", pthread_self(), 
                     (char*)this->arg_);
             sleep(1);
             return 0;
        }
};


int main()
{
    char szTmp[] = "hello world";

    MyTask taskobj;
    taskobj.setArg((void*)szTmp);

    z1::ThreadPool threadPool(10);
    for(int i = 0; i < 20; i++)
    {
        threadPool.addTask(&taskobj);
    }

    while(1)
    {
        printf("There are still %d tasks need to process\n",threadPool.size());
        if(threadPool.size() == 0)
        {
            threadPool.stop();
            printf("Now I will exit from main\n");
            exit(0);
        }
        sleep(2);
    }

    return 0;
} 
