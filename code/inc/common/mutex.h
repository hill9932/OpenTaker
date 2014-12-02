#ifndef __VP_MUTEX_INCLUDE_H__
#define __VP_MUTEX_INCLUDE_H__

#include "common.h"
#include "mutex_i.h"

class Mutex : public ILockable
{
public:
    Mutex();
    virtual ~Mutex();
    virtual void lock();
    virtual void unlock();
    virtual void init();

private:
    Mutex (const Mutex&);
    Mutex& operator= (const Mutex&);

private:
    CRITICAL_SECTION mId;
};


class MutexWrap
{
public:
    MutexWrap(Mutex& _mutex) : m_mutex(_mutex)
    {
        m_mutex.lock();
    }

    ~MutexWrap()
    {
        m_mutex.unlock();
    }
private:
    Mutex& m_mutex;
};


class MutexWrap_
{
public:
    MutexWrap_(CRITICAL_SECTION& _mutex);
    ~MutexWrap_();

private:
    CRITICAL_SECTION& m_mutex;
};

void InitCriticalSec(CRITICAL_SECTION& _cs);

#define SCOPE_LOCK(lock)    MutexWrap locker(lock);
#define SCOPE_LOCK_(lock)   MutexWrap_ locker(lock);

#endif
