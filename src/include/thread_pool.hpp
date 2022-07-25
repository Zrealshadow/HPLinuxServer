#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <list>
#include "stdio.h"
#include <cstdlib>
#include "locker.h"

template <typename T>
class threadpool
{
public:
    threadpool(int thread_number = 8, int max_requests = 1000);
    ~threadpool();
    bool append(T *request);

private:
    static void *work(void *arg); // thread run func
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    pthread_t *m_threads;
    std::list<T *> m_workqueue;
    locker m_queuelocker;
    sem m_queuestat;
    bool m_stop;
};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests)
    : m_thread_number(thread_number), m_max_requests(m_max_requests), m_stop(false), m_threads(nullptr)
{
    if ((thread_number <= 0) || (m_max_requests <= 0))
    {
        throw std::exception();
    }
    m_threads = new pthread_t[thread_number];
    if (!m_threads)
    {
        throw std::exception();
    }

    for (int i = 0; i < m_thread_number; ++i)
    {
        printf("create the %d th thread\n", i);
        if (pthread_create(m_threads + i, NULL, work, this) != 0)
        {
            // !=0 failed
            delete[] m_threads;
            throw std::exception();
        }

        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T * request){
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests){
        // the queue size is larger than max requests
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    // trigger thread which waiting for request
    return true;
}

template<typename T>
void * threadpool<T>::work(void *arg){
    threadpool * pool = (threadpool *) arg;
    pool->run();
    return pool;
}


template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();
        // if the queue has request, it will be triggered
        
        // add lock, get request from shared queue
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            // restart to wait for sem
            m_queuelocker.unlock();
            continue;
        }

        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request){
            // if this request is an empty ptr
            continue;
        }

        // request is the generic T, it should implement process interface
        request->process();
    }
}
#endif