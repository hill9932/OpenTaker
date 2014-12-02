/**
* @Function:
*  This file defines the class the hold fix size buffer pool.
* @Memo:
*  Create by hill, 4/29/2014
**/

#ifndef __VP_FIXSIZE_BUFFER_INCLUDE_H__
#define __VP_FIXSIZE_BUFFER_INCLUDE_H__

#include "common.h"
#include "system_.h"

template<class T>
class IBufferPool
{
public:
    virtual ~IBufferPool()    {}
    virtual T* getBlock() = 0;
    virtual T* tryGetBlock() = 0;
    virtual bool release(T* _block) = 0;
    virtual u_int32 getBlockCount() = 0;
};


template<class T>
class CFixBufferPool : public IBufferPool<T>
{
public:
    CFixBufferPool(u_int32 _count = 0, T* _mem = NULL)
    {
        m_bufAddr = m_nextBlock = NULL;
        m_bufCount = 0;
        m_allocate = false;
        m_stop     = false;
        resetMem(_count);
    }

    ~CFixBufferPool()
    {
        if (m_allocate && m_bufAddr) delete[] m_bufAddr;
    }

    void stop(bool _stop)
    {
        m_stop = _stop;
    }

    void resetMem(int _count)
    {
        if (_count > 0)
        {
            if (m_allocate && m_bufAddr) delete[] m_bufAddr;

            m_bufCount = _count;
            m_bufAddr = new T[_count];
            m_allocate = true;
            init();
        }
    }

    void resetMem(T* _mem, int _count)
    {
        if (!_mem || _count < 0)
        {
            RM_LOG_ERROR("The memory is invalid.");
            return;
        }

        if (m_allocate && m_bufAddr) delete[] m_bufAddr;
        m_bufAddr = _mem;
        m_bufCount = _count;
        init();
    }

    u_int32 getBlockCount()
    {
        return m_bufCount;
    }

    T* getBlock()
    {
        int idleTime = 0;
        T* block = tryGetBlock();
        while (!block && !m_stop)
        {
            block = tryGetBlock();
            if (!block && ++idleTime % 1000 == 0)   SleepMS(MyMin(idleTime / 1000, 1000));
            else YieldCurThread();
        }

        return block;
    }

    bool checkValid(T* _block)
    {
        bool ret = false;
        for (int i = 0; i < m_bufCount; ++i)
        {
            if (!ret)   ret = _block == &m_bufAddr[i];
            else break;
        }

        assert(ret);
        return ret;
    }

    bool release(T* _block)
    {
        if (!_block)
        {
            assert(false);
            return false;
        }

#if defined(DEBUG) || defined(_DEBUG)
        assert(checkValid(_block));
#endif

        SCOPE_LOCK(m_bufMutex);
        ((Block_t*)_block)->m_next = m_nextBlock;
        m_nextBlock = _block;

#if defined(DEBUG) || defined(_DEBUG)
        INTERLOCKED_INCREMENT(&m_blockReleaseCount);
#endif

        return m_nextBlock != NULL;
    }

    T* tryGetBlock()
    {
        SCOPE_LOCK(m_bufMutex);
        T* block = m_nextBlock;
        if (m_nextBlock) m_nextBlock = ((Block_t*)m_nextBlock)->m_next;
#if defined(DEBUG) || defined(_DEBUG)
        if (block) INTERLOCKED_INCREMENT(&m_blockUsedCount);
#endif
        return block;
    }

    union Block_t
    {
        T   data;
        T*  m_next;
    };

private:
    void init()
    {
        if (!m_bufAddr) return;

        Block_t* block = (Block_t*)&m_bufAddr[0];
        for (int i = 1; i < m_bufCount; ++i)
        {
            block->m_next = &m_bufAddr[i];
            block = (Block_t*)block->m_next;
        }

        block->m_next = NULL;
        m_nextBlock = &m_bufAddr[0];
#if defined(DEBUG) || defined(_DEBUG)
        m_blockReleaseCount = m_blockUsedCount = 0;
#endif
    }

private:
    T*      m_bufAddr;
    T*      m_nextBlock;
    u_int32 m_bufCount;

#if defined(DEBUG) || defined(_DEBUG)
    atomic_t m_blockUsedCount;
    atomic_t m_blockReleaseCount;
#endif

    Mutex   m_bufMutex;
    bool    m_allocate;
    bool    m_stop;
};

#endif
