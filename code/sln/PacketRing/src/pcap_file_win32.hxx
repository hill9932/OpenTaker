int CPCAPFile::setFileSize(u_int64 _fileSize)
{
    if (INVALID_HANDLE_VALUE == m_fileHandle) return -1;

    SetFilePointer(m_fileHandle, _fileSize, NULL, FILE_BEGIN);
    SetEndOfFile(m_fileHandle);

    m_fileSize = _fileSize;
}

int CPCAPFile::createMapFile(CStdString& _fileName, u_int64 _fileSize)
{
    //
    // set the file size
    //
    m_fileHandle = CreateFile(_fileName,
        _fileSize > 0 ? GENERIC_WRITE | GENERIC_READ : GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        _fileSize > 0 ? OPEN_ALWAYS : OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        return GetLastSysError(); 
    }

    u_int32 low32Size = 0;
    if (_fileSize != 0) setFileSize(_fileSize);
    else
    {
        m_fileSize = 0;
        low32Size = getSize((u_int32*)&m_fileSize);
        m_fileSize <<= 32;
        m_fileSize += low32Size;
    }

    //
    // create file share map
    //
    m_fileView = CreateFileMapping(m_fileHandle,
        NULL,
        _fileSize > 0 ? PAGE_READWRITE : PAGE_READONLY,
        0,
        0,
        NULL);
    if (m_fileView == NULL)
    {
        return GetLastSysError();
    }

    m_buf = (byte*)MapViewOfFile(m_fileView, 
                                 _fileSize > 0 ? FILE_MAP_WRITE | FILE_MAP_READ : FILE_MAP_READ, 
                                 0, 0, 0);
    return GetLastSysError();
}


int CPCAPFile::close()
{
    if (isValid())
    {
        FlushViewOfFile(m_buf, 0);
        FlushFileBuffers(m_fileHandle);
        UnmapViewOfFile(m_buf);
        m_buf = NULL;
    }

    if (m_fileView)   
    {
        CloseHandle(m_fileView);
        m_fileView = NULL;
    }

    if (m_fileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_fileHandle);
        m_fileHandle = INVALID_HANDLE_VALUE;
    }

    m_offset = 0;
    return 0;
}
