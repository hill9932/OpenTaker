/**
 * @Function: 
 *  This file define the class of ShareMemory which allocates a large range of memory to be 
 *  shared by other processes.
 * @Memo:
 *  Created by hill, 4/15/2014
 **/

#ifndef __HILUO_SHARE_MEMORY_INCLUDE_H__
#define __HILUO_SHARE_MEMORY_INCLUDE_H__

template<class T>
class CShareMemory
{
public:
    CShareMemory()
    {
        m_memSize   = 0;
        m_memStart  = NULL;
        m_memObj    = NULL;
    }

    ~CShareMemory()
    {
        destroy();
    }

    bool create(u_int64 _memSize);

    void destroy()
    {
        if (m_memObj)
        {
            CloseHandle(m_memObj);
            m_memObj = NULL;
        }
    }

private:
    u_int64     m_memSize;      // the number of T block of this shared memory
    handle_t    m_memObj;
    T*          m_memStart;     // the start address of the shared memory
    // volatile T* m_memWritePtr;  // the write position
    // volatile T* m_memReadPtr;   // the read position
};

#ifdef WIN32
#include "share_memory_win32.hxx"
#else
#include "share_memory_linux.hxx"
#endif

#endif
