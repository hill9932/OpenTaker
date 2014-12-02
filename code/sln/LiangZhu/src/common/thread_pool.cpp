#include "thread_pool.h"

#include <boost/thread/thread.hpp>

using namespace boost;


CThread::CThread(int64 _check_time_ms) : thread(doTask, this), m_queue_sem(0)
{
    m_enable = true;
    m_check_time_ms = _check_time_ms;
}

CThread::~CThread()
{
    m_enable = false;
    join();
}

void CThread::addTask(ICallable* _task, bool _high_priority /* = false */)
{
    if (m_enable)
    {
        SCOPE_LOCK(m_queue_mutex);
        _high_priority ? m_task_queue.push_front(_task) : m_task_queue.push_back(_task);

        m_queue_sem.post();
    }
}

/**
 * @Function: every thread will call this to handle all the tasks in the queue
 **/
int CThread::doTask(CThread* _thread_obj)
{
    try
    {
        while(_thread_obj->enabled())
        {
            // by default check the enable flag every 3 seconds
            if (_thread_obj->exec() == 0 &&
               (_thread_obj->m_queue_sem.timed_wait(get_system_time() + posix_time::millisec(_thread_obj->m_check_time_ms))))
            {
                ICallable* task = NULL;
                {
                    SCOPE_LOCK(_thread_obj->m_queue_mutex);
                    task = _thread_obj->m_task_queue.front();
                    _thread_obj->m_task_queue.pop_front();
                }

                if (task)
                {
                    (*task)();
                }
            }
        }
    }
    catch(...)
    {
        RM_LOG_ERROR("Capture unexpected exception.");
    }

    _thread_obj->clear();

    return 0;
}

/**
 * @Function: clear the task queue and delete all the task objects
 * @Memo: can only be called when thread willing to stop
 **/
void CThread::clear()
{
    SCOPE_LOCK(m_queue_mutex);
    ICallable* task = NULL;

    list<ICallable*>::iterator it = m_task_queue.begin();
    for (; it != m_task_queue.end(); ++it)
    {
        task = *it;
        delete task;
    }

    m_task_queue.clear();

    while(m_queue_sem.try_wait());
}


/**********************************/
//     CThreadPool
/**********************************/
int CThreadPool::getCPUNumber()
{
    int num = boost::thread::hardware_concurrency();
    return num;
}

CThreadPool::CThreadPool(int _thread_num, bool _create_thread)
{
    if (!_create_thread)    return;

    int thread_num = _thread_num;

    if (-1 == thread_num)
    {
        thread_num = getCPUNumber();
        //RM_INFO("Total " << thread_num << " CPUs");

        thread_num *= 2;
        thread_num += 3;
        if (thread_num < 2)  thread_num = 4;
    }

    for (int i = 0; i < thread_num; ++i)
    {
        CThread* thread = new CThread;
        add_thread(thread);
        m_threads.push_back(thread);

        RM_LOG_DEBUG_S("New thread created: " << dec << thread->get_id());
    }
}

/**
 * @Function: stop all the threads and delete the thread objects
 **/
CThreadPool::~CThreadPool()
{
    stop();
    join_all();

    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        CThread* thread = m_threads[i];
        remove_thread(thread);
        delete thread;
    }
    m_threads.clear();
}

/**
 * @Function: stop the thread to do task and stop running
 **/
void CThreadPool::stop()
{
    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        CThread* thread = m_threads[i];
        thread->enable(false);
        thread->join();
    }
}

/**
 * @Function: add task to the thread's queue who has the most capability to handle it
 *  if no thread is available, just delete it
 **/
void CThreadPool::addTask(ICallable* _task, bool _high_priority)
{
    CThread* thread = NULL;
    int min_size = 0x7FFFFFFF;

    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        if (!m_threads[i]->enabled()) continue;

        int size = m_threads[i]->size();
        if (size == 0)
        {
            thread = m_threads[i];
            break;
        }

        if (size < min_size)
        {
            thread = m_threads[i];
        }
    }

    if (thread)
        thread->addTask(_task, _high_priority);
    else
    {
        delete _task;
    }
}

vector<CThread*>& CThreadPool::detach()
{
    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        remove_thread(m_threads[i]);
    }

    return m_threads;
}
