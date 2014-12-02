#ifdef WIN32

int CFilePool::getFinishedRequest(vector<PPER_FILEIO_INFO_t>& _request)
{
    PPER_FILEIO_INFO_t ioContext = NULL;
    DWORD dwBytes = 0;
    ULONG_PTR completionKey = 0;
    int rv = 0;

    while (true)
    {
        ioContext = NULL;
        BOOL ret = GetQueuedCompletionStatus(
            m_IOService.m_aioHandler,
            &dwBytes,
            &completionKey,
            (LPOVERLAPPED*)&ioContext,
            0);

        if (!ret && ioContext && GetLastSysError() != WAIT_TIMEOUT)
        {
            ioContext->flag = GetLastSysError();
            _request.push_back(ioContext);
        }
        else if (ret && ioContext)
        {
            _request.push_back(ioContext);
        }
        else
            break;
    } ;

    return _request.size();
}

int CFilePool::getFinishedRequest()
{
    PPER_FILEIO_INFO_t ioContext = NULL;
    DWORD dwBytes = 0;
    ULONG_PTR completionKey = 0;
    int count = 0;

    while (true)
    {
        ioContext = NULL;
        BOOL ret = GetQueuedCompletionStatus(
            m_IOService.m_aioHandler,
            &dwBytes,
            &completionKey,
            (LPOVERLAPPED*)&ioContext,
            0);

        if (m_aioCallback && ioContext)
        {
            ioContext->flag = ret ? 0 : GetLastSysError();
            m_aioCallback(m_context, ioContext);
        }
        else if (!ret && ioContext && GetLastSysError() != WAIT_TIMEOUT)
        {
            ioContext->flag = GetLastSysError();
            if (m_errorCallback) m_errorCallback(m_context, ioContext, ioContext->flag);
            ++count;
        }
        else if (ret && ioContext)
        {
            if (FILE_READ == ioContext->optType)
            {
                if (m_readCallback) m_readCallback(m_context, ioContext, ioContext->dataLen);
            }
            else if (FILE_WRITE == ioContext->optType)
            {
                if (m_writeCallback)m_writeCallback(m_context, ioContext, ioContext->dataLen);
            }

            ++count;
        }
        else
            break;
    };

    return count;
}

int CFilePool::IoHandler::operator()()
{
    while (!m_filePool->m_isStop)
    {
        PPER_FILEIO_INFO_t io_context = NULL;
        DWORD dwBytes = 0;
        ULONG_PTR completion_key = 0;
        int rv = 0;

        BOOL ret = GetQueuedCompletionStatus(
            m_filePool->m_IOService.m_aioHandler,
            &dwBytes,
            &completion_key,
            (LPOVERLAPPED*)&io_context,
            1000);
        m_total++;
        u_int32 err = GetLastSysError();
        if (!ret && io_context && err != WAIT_TIMEOUT)
        {
            m_totalError++;

            LOG_LAST_ERROR_MSG();
            if (m_filePool->m_errorCallback)
                rv = m_filePool->m_errorCallback(m_filePool->m_context, io_context, err);

            if (rv == 0)
                m_filePool->release(io_context);
        }
        else if (ret && io_context)
        {
            u_int32 offset = io_context->Offset;
            int index = io_context->index;
            io_context->dataLen = dwBytes;

            if (FILE_READ == io_context->optType)
            {
                m_totalRead++;
                if (m_filePool->m_readCallback)
                {
                    rv = m_filePool->m_readCallback(this->m_filePool->m_context, io_context, io_context->dataLen);
                }
            }
            else if (FILE_WRITE == io_context->optType)
            {
                m_totalWrite++;
                if (m_filePool->m_writeCallback)
                {
                    rv = m_filePool->m_writeCallback(this->m_filePool->m_context, io_context, io_context->dataLen);
                }
            }

            if (rv == 0)
                m_filePool->release(io_context);
        }
        else if (err != WAIT_TIMEOUT)    // mostly signal by PostQueuedCompletionStatus
        {
            return -1;
        }
    }

    //ADD_TASK(this);

    return 0;
}



#endif
