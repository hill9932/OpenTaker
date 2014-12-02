#include "mutex.h"

Mutex::Mutex()
{
    init();
}


Mutex::~Mutex ()
{
#ifndef WIN32
    int  rc = pthread_mutex_destroy(&mId);
    (void)rc;
    assert( rc != EBUSY );  // currently locked 
    assert( rc == 0 );
#else
    DeleteCriticalSection(&mId);
#endif
}

void Mutex::init()
{
    InitCriticalSec(mId);
}


void Mutex::lock()
{
#ifdef WIN32
    EnterCriticalSection(&mId);
#else
    int  rc = pthread_mutex_lock(&mId);
    (void)rc;
    assert( rc != EINVAL );
    assert( rc != EDEADLK );
    assert( rc == 0 );    
#endif
}

void Mutex::unlock()
{
#ifdef WIN32
    LeaveCriticalSection(&mId);
#else
    int  rc = pthread_mutex_unlock(&mId);
    (void)rc;
    assert( rc != EINVAL );
    assert( rc != EPERM );
    assert( rc == 0 );    
#endif
}

MutexWrap_::MutexWrap_(CRITICAL_SECTION& _mutex) 
: m_mutex(_mutex)
{
#ifdef WIN32
    EnterCriticalSection(&m_mutex);
#else
    int rc = pthread_mutex_lock(&m_mutex);
#endif
}

MutexWrap_::~MutexWrap_()
{
#ifdef WIN32
    LeaveCriticalSection(&m_mutex);
#else
    int rc = pthread_mutex_unlock(&m_mutex);
#endif 
}


void InitCriticalSec(CRITICAL_SECTION& _cs)
{
#ifdef WIN32
    InitializeCriticalSection(&_cs);
#else
    int  rc = pthread_mutex_init(&_cs,0);
    (void)rc;
    assert( rc == 0 );
#endif
}


