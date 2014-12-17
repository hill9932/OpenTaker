/**
 * @Function:
 *  The object in this pool will be allocated in sequence
 **/
#ifndef __HILUO_OBJEC_BUFFER_INCLUDE_H__
#define __HILUO_OBJEC_BUFFER_INCLUDE_H__

#include "common.h"
#include "system_.h"
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

namespace LiangZhu
{
    /**
     * @Function: A object pool with fixed size
     **/
    template <class T>
    class CObjectPool
    {
    public:
        CObjectPool(u_int32 _count)
        {
            m_curIndex = 0;
            m_objCount = _count;
            m_objBuf = new InteralObject[m_objCount];
            for (int i = 0; i < m_objCount; ++i)
            {
                m_objBuf[i].used = 0;
                m_objBuf[i].index = i;
            }
        }

        virtual ~CObjectPool()
        {
            delete[] m_objBuf;
        }

        virtual T* getObject()
        {
            int idleTime = 0;

        retry:
            u_int32 curIndex = m_curIndex;
            u_int32 nextIndex = (curIndex + 1) % m_objCount;
            InteralObject* obj = &m_objBuf[curIndex];
            if (obj->used != 0)
            {
                YieldCurThread();
                goto retry;
            }

            if (curIndex != boost::interprocess::ipcdetail::atomic_cas32(&m_curIndex, nextIndex, curIndex))   // move to the next
                goto retry;

            obj->used = 1;
            return (T*)obj;
        }

        virtual void release(T* _obj)
        {
            if (_obj)
            {
                InteralObject* obj = (InteralObject*)_obj;
                obj->used = 0;
            }
        }

    protected:
        struct InteralObject
        {
            T   object;
            u_int32 used    : 1;
            u_int32 index   : 31;
        };

        InteralObject*  m_objBuf;
        u_int32         m_objCount;
        volatile u_int32  m_curIndex;
    };


    /**
     * @Function:
     *
     **/
    template <class T>
    class CObjectPool3 : public CObjectPool<T>
    {
    public:
        CObjectPool3(u_int32 _objCount) : CObjectPool<T>(_objCount)
        {
        }

        virtual T* getObject()
        {
        retry:
            u_int32 curIndex = this->m_curIndex;
            u_int32 nextIndex = (curIndex + 1) % this->m_objCount;
            typename CObjectPool<T>::InteralObject* obj = &this->m_objBuf[curIndex];

            while (obj->used != 0)
            {
                obj = &this->m_objBuf[nextIndex++];
            }

            if (curIndex != boost::interprocess::ipcdetail::atomic_cas32(&this->m_curIndex, nextIndex, curIndex))   // move to the next
                goto retry;

            obj->used = 1;
            return (T*)obj;
        }
    };    
}

#endif
