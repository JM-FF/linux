#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

//描述线程池的结构体重新命名
typedef struct threadpool_t threadpool_t;

//线程池的创建
threadpool_t* threadpool_create(int min_thr_num, int max_thr_num,int queue_max_size);

//向线程池中增加任务
int threadpool_add(threadpool_t* pool,void*(*function)(void *arg),void *arg);

//销毁线程池
int threadpool_destory(threadpool_t* pool);

//获取线程池中存活的线程数量
int threadpool_all_threadnum(threadpool_t *pool);

//获取线程池中忙线程的数量
int thradpool_busy_threadnum(threadpool_t *pool);

#endif
