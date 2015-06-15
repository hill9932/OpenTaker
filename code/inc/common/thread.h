/**
 * @Function:
 *  This file define thread related things.
 * @Memo:
 *  Created by hill, 4/13/2014
 **/
#ifndef __HILUO_THREAD_INCLUDE_H__
#define __HILUO_THREAD_INCLUDE_H__

#include "common.h"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>

namespace LiangZhu
{
#if defined(PTHREADS)
    typedef pthread_key_t ThreadSpecificKey;

    static int CreateThreadKey(ThreadSpecificKey* _key, void(*destructor)(void *))
    {
        int err = pthread_key_create(_key, destructor);
        ON_ERROR_LOG_LAST_ERROR(err, !=, 0);
        return err;
    }

    static int DeleteThreadKey(ThreadSpecificKey _key)
    {
        int err = pthread_key_delete(_key);
        ON_ERROR_LOG_LAST_ERROR(err, !=, 0);
        return err;
    }

    static void SetThreadValueWithKey(ThreadSpecificKey _key, void* _value)
    {
        pthread_setspecific(_key, _value);
    }

    static void* GetThreadValueWithKey(ThreadSpecificKey _key)
    {
        return pthread_getspecific(_key);
    }

#else
    typedef int ThreadSpecificKey;

    static int CreateThreadKey(ThreadSpecificKey* _key, void(*destructor)(void *))
    {
        *_key = TlsAlloc();
        return 0;
    }

    static int DeleteThreadKey(ThreadSpecificKey _key)
    {
        TlsFree(_key);
        return 0;
    }

    static void SetThreadValueWithKey(ThreadSpecificKey _key, void* _value)
    {
        TlsSetValue(_key, _value);
    }

    static void* GetThreadValueWithKey(ThreadSpecificKey _key)
    {
        return TlsGetValue(_key);
    }

#endif
    

    typedef int(*Thread_Main)(void*);

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
}
