#ifndef __HILUO_THREADPOOL_INCLUDE_H__
#define __HILUO_THREADPOOL_INCLUDE_H__

#include "common.h"
#include "mutex.h"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/asio.hpp>
#include <list>
#include <vector>


class CThread : public boost::thread
{
public:
    CThread(int64 _check_time_ms = 3000);
    virtual ~CThread();

    static int doTask(CThread* _thread_obj);

    void addTask(ICallable* _task, bool _high_priority = false);
    int  size()  { return m_task_queue.size(); }
    void enable(bool _enable) { m_enable = _enable; }
    bool enabled() { return m_enable; }

    friend class CThreadPool;

private:
    void clear();

    // return 0 to continue handle the queues, otherwise ignore the queue
    virtual int exec() { return 0; }

private:
    boost::interprocess::interprocess_semaphore  m_queue_sem;
    std::list<ICallable*>   m_task_queue;
    Mutex                   m_queue_mutex;
    volatile bool           m_enable;
    int64                   m_check_time_ms;    // mil-second
};


class CThreadPool : public boost::thread_group
{
public:
    CThreadPool(int _thread_num = 0, bool _create_thread = true);
    ~CThreadPool();

    int  getThreadNum(){ return m_threads.size(); }
    void addTask(ICallable* _task, bool _high_priority = false);
    void stop();
    vector<CThread*>& detach();

protected:
    int getCPUNumber();

private:
    std::vector<CThread*>  m_threads;
};
#endif
