#include "file_pool.h"

#define ADD_TASK(task)  m_filePool->m_threadPool.addTask(task);

/*
CFilePool::WriteCompleteCallback CFilePool::s_write_callback;
CFilePool::ReadCompleteCallback  CFilePool::s_read_callback;
CFilePool::ErrorCallback         CFilePool::s_error_callback;
*/

CFilePool::CFilePool(int _blockNum, int _threadNum, void* _context)
: m_threadPool(MyMin(_threadNum, 20))
, m_IOContextPool(MyMax(_blockNum, 1024))
{
    m_readCallback  = boost::bind(&CFilePool::onDataRead,    this, _1, _2, _3);
    m_writeCallback = boost::bind(&CFilePool::onDataWritten, this, _1, _2, _3);
    m_errorCallback = boost::bind(&CFilePool::onError,       this, _1, _2, _3);
    m_context        = _context;
    m_isStop         = false;
    m_aioCallback   = NULL;
}

CFilePool::~CFilePool()
{
    stop();
}

int CFilePool::start(ICallable* _handler)
{
    ICallable* task = NULL;
    for (int i = 0; i < m_threadPool.getThreadNum(); i++)
    {
        task = _handler ? _handler : new IoHandler(this);
        m_threadPool.addTask(task);
    }

    return 0;
}

int CFilePool::stop()
{
    m_isStop = true;
    m_threadPool.stop();

    for (int i = 0; i < MyMax(m_threadPool.getThreadNum() * 2, 2); i++)
    {
#ifdef WIN32
        PostQueuedCompletionStatus(m_IOService.m_aioHandler, 0, NULL, NULL);
#else
        //raise(SIGUSR1);
#endif
    }

    return 0;
}

void CFilePool::wait()
{
    m_threadPool.join_all();
}

int CFilePool::attach(handle_t _fileHandle, void* _context, u_int32 _flag)
{
    Aio_t h = m_IOService.assoicateObject(_fileHandle, _context, _flag);
    int rv = -1;

    if (h == NULL)
    {
        rv = GetLastSysError();
        LOG_ERROR_MSG(0);
    }
    else
    {
        rv = 0;
    }

    return rv;
}

int CFilePool::attach(CHFile& _file,  void* _context, u_int32 _flag)
{
    return attach((handle_t)_file, _context, _flag);
}

int CFilePool::onDataRead(void* _handler, PPER_FILEIO_INFO_t _io_context, u_int32 _bytes_count)
{
    return 0;
}

int CFilePool::onDataWritten(void* _handler, PPER_FILEIO_INFO_t _io_context, u_int32 _bytes_count)
{
    return 0;
}

int CFilePool::onError(void* _handler, PPER_FILEIO_INFO_t _io_context, int _error_code)
{
    return 0;
}


#ifdef WIN32
#include "file_pool_win32.hxx"
#else
#include "file_pool_linux.hxx"
#endif
