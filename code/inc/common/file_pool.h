#ifndef __HILUO_FILEPOOL_INCLUDE_H__
#define __HILUO_FILEPOOL_INCLUDE_H__

#include "file.h"
#include "aio_.h"
#include "thread_pool.h"
#include "fixsize_buffer.h"
#include "object_buffer.hxx"
#include <boost/function.hpp>

namespace LiangZhu
{
    typedef boost::function<int(void*, PPER_FILEIO_INFO_t, u_int32)>  CompleteCallback;
    typedef boost::function<int(void*, PPER_FILEIO_INFO_t, u_int32)>  WriteCompleteCallback;
    typedef boost::function<int(void*, PPER_FILEIO_INFO_t, u_int32)>  ReadCompleteCallback;
    typedef boost::function<int(void*, PPER_FILEIO_INFO_t, int)>      ErrorCallback;
    typedef int(*CallBack_Func) (void*, PPER_FILEIO_INFO_t);

    class CFilePool
    {
    public:
        /**
         * @Param _blockNum:    the number of fixed size block
         * @Param _threadNum:   the number of thread to handle finished io
         * @Param _context:     user defined parameter which will be the void* value in callback
         **/
        CFilePool(int _blockNum, int _threadNum, void* _context = NULL);
        ~CFilePool();

        int  start(ICallable* _handler);
        int  stop();
        int  attach(CHFile& _file, void* _context = NULL, u_int32 _flag = 0);
        int  attach(handle_t _file_handle, void* _context = NULL, u_int32 _flag = 0);
        void wait();

        int getFinishedRequest(vector<PPER_FILEIO_INFO_t>& _request);
        int getFinishedRequest();
        Aio_t getAioObject() { return m_IOService.m_aioHandler; }

        void setWriteCompleteCallback(WriteCompleteCallback _callback)
        {
            m_writeCallback = _callback;
        }

        void setReadCompleteCallback(ReadCompleteCallback  _callback)
        {
            m_readCallback = _callback;
        }

        void setErrorCallback(ErrorCallback _callback)
        {
            m_errorCallback = _callback;
        }

        void setAioCallback(CallBack_Func _callback)
        {
            m_aioCallback = _callback;
        }

        inline PPER_FILEIO_INFO_t getBlock()
        {
            //        return m_IOContextPool.getBlock();      
            return m_IOContextPool.getBlock();
        }

        inline void release(PPER_FILEIO_INFO_t _block)
        {
            m_IOContextPool.release(_block);
        }

        inline PPER_FILEIO_INFO_t tryGetBlock()
        {
            return m_IOContextPool.getBlock();
            //        return m_IOContextPool.tryGetBlock();   
        }


    private:
        struct IoHandler : public ICallable
        {
            IoHandler(CFilePool* _filePool)
            {
                m_filePool = _filePool;
                m_total = m_totalWrite = m_totalRead = m_totalError = 0;
            }

            ~IoHandler()
            { }

            int operator()();

        private:
            CFilePool*  m_filePool;
            u_int32     m_total;
            u_int32     m_totalWrite;
            u_int32     m_totalError;
            u_int32     m_totalRead;
        };

    protected:
        virtual int onDataRead(void*, PPER_FILEIO_INFO_t, u_int32);
        virtual int onDataWritten(void*, PPER_FILEIO_INFO_t, u_int32);
        virtual int onError(void*, PPER_FILEIO_INFO_t, int);

    public:
        WriteCompleteCallback m_writeCallback;
        ReadCompleteCallback  m_readCallback;
        ErrorCallback         m_errorCallback;
        CallBack_Func         m_aioCallback;

    private:
        CAsyncIO        m_IOService;
        CThreadPool     m_threadPool;       // thread pool
        //CFixBufferPool<PER_FILEIO_INFO_t>   m_IOContextPool;
        CObjectPool3<PER_FILEIO_INFO_t> m_IOContextPool;

        bool            m_isStop;
        void*           m_context;  // user will set this value to indicate more values
    };

}

#endif
