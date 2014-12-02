#include "thread.h"

CSimpleThread::CSimpleThread(ThreadFun& _threadFun)
: thread(doTask2, this), m_func(_threadFun)
{
    m_context = NULL;
    m_enable = true;
}

CSimpleThread::CSimpleThread(Thread_Main _func, void* _context) 
: thread( _func ? _func : doTask, _context ? _context : this)
{
    m_context = _context;
    m_enable  = true;
}

CSimpleThread::~CSimpleThread()
{
    m_enable = false;
}

int CSimpleThread::doTask(void* _threadObj)
{
    CSimpleThread* obj = (CSimpleThread*)_threadObj;
    return obj->doTaskImpl();
}

int CSimpleThread::doTaskImpl()
{
    return 0;
}

int CSimpleThread::doTask2(void* _threadObj)
{
    CSimpleThread* obj = (CSimpleThread*)_threadObj;
    obj->m_func();
    delete obj;
    return 0;
}
