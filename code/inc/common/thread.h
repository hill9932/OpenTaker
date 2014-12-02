/**
 * @Function:
 *  This file define thread related things.
 * @Memo:
 *  Created by hill, 4/13/2014
 **/
#ifndef __VP_THREAD_INCLUDE_H__
#define __VP_THREAD_INCLUDE_H__

#include "common.h"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>

class CSimpleThread;
typedef int (*Thread_Main)(void*);

class CSimpleThread : public boost::thread
{
public:
    typedef boost::function<void()> ThreadFun;

    CSimpleThread(ThreadFun& _threadFun);
    CSimpleThread(Thread_Main _func = NULL, void* _context = NULL);
    virtual ~CSimpleThread();

    void enable(bool _enable) { m_enable = _enable; }
    bool enable() { return m_enable; }

protected:
    static int doTask(void* _threadObj);
    static int doTask2(void* _threadObj);

    virtual int doTaskImpl();   // derived class will override this to do real the job

public:
    void* m_context;

private:
    volatile bool m_enable;
    ThreadFun     m_func;             // thread function
};

#endif