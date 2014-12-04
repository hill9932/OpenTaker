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
    // FIFO object pool
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
                //if (++idleTime % 1000 == 0) SleepMS(MyMin(idleTime / 1000, 1000));
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
            u_int32 used;
            u_int32 index;
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


    template <class T>
    struct CObjectNode
    {
        T				object;
        CObjectNode*    nextNode;
    };

    // object pool for not FIFO use, can be get from one thread or tbb task and release obj or list from another thread or task
    template <class T>
    class CObjectPool2
    {
    public:
        CObjectPool2(u_int32 _blockSize = 1024, u_int32 _maxBlockSize = 1024 * 1024)
            : m_blockSize(_blockSize), m_MAX_BLOCK_SIZE(_maxBlockSize), m_curIndex(_blockSize), m_firstDeleteNode(NULL), m_lastDeleteNode(NULL), m_objBlkList(NULL)
        {
            assert(_blockSize > 0);
        }

        virtual ~CObjectPool2()
        {
            while (m_objBlkList != NULL)
            {
                ObjectBlock* curBlk = m_objBlkList;
                m_objBlkList = curBlk->nextBlock;
                if (curBlk->objArray != NULL)
                {
                    delete[] curBlk->objArray;
                }
                delete curBlk;
            }
        }

        typedef CObjectNode<T> ObjectNode;

        T* getObject()
        {
            if (m_firstDeleteNode != NULL && m_firstDeleteNode != m_lastDeleteNode)
            {
                //boost::lock_guard<boost::mutex> lock(m_mutex);
                ObjectNode* cur = m_firstDeleteNode;
                m_firstDeleteNode = m_firstDeleteNode->nextNode;
                return &(cur->object);
            }
            if (m_curIndex >= m_blockSize)
            {
                if (!newBlock())
                {
                    return NULL;
                }
            }
            assert(m_blockSize > m_curIndex);
            return &(m_objBlkList->objArray[m_curIndex++].object);
        }

        // this can be called from another thread or task
        void releaseObject(T* _obj)
        {
            //boost::lock_guard<boost::mutex> lock(m_mutex);
            if (m_lastDeleteNode == NULL)
            {
                assert(m_firstDeleteNode == NULL);
                m_lastDeleteNode = (ObjectNode*)_obj;
                m_firstDeleteNode = (ObjectNode*)_obj;
            }
            else
            {
                m_lastDeleteNode->nextNode = (ObjectNode*)_obj;
                m_lastDeleteNode = (ObjectNode*)_obj;
            }
        }

        // this can be called from another thread or task
        void releaseObjectList(ObjectNode* _from, ObjectNode* _to)
        {
            //boost::lock_guard<boost::mutex> lock(m_mutex);
            if (m_lastDeleteNode == NULL)
            {
                assert(m_firstDeleteNode == NULL);
                m_lastDeleteNode = _to;
                m_firstDeleteNode = _from;
            }
            else
            {
                m_lastDeleteNode->nextNode = _from;
                m_lastDeleteNode = _to;
            }
        }

    private:
        struct ObjectBlock
        {
            ObjectNode*     objArray;
            ObjectBlock*    nextBlock;
        };

        bool newBlock()
        {
            if (m_blockSize < m_MAX_BLOCK_SIZE && m_objBlkList)
            {
                m_blockSize <<= 1;
            }
            ObjectBlock* pBlock = new ObjectBlock();
            if (pBlock == NULL)
            {
                return false;
            }
            pBlock->nextBlock = m_objBlkList;
            m_objBlkList = pBlock;
            m_objBlkList->objArray = new ObjectNode[m_blockSize];
            if (m_objBlkList->objArray == NULL)
            {
                assert(false);
                return false;
            }
            m_curIndex = 0;
            return true;
        }

        ObjectBlock*    m_objBlkList;
        ObjectNode*     m_firstDeleteNode;
        ObjectNode*     m_lastDeleteNode;
        u_int32         m_blockSize;
        u_int32         m_curIndex;
        const u_int32   m_MAX_BLOCK_SIZE;
        //boost::mutex    m_mutex;
    };
}

#endif
