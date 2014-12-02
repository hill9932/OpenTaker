#ifdef WIN32

template<class T>
bool CShareMemory<T>::create(u_int64 _memSize)
{
    if (_memSize <= 0 ) return false;
    m_memObj = CreateFileMapping(INVALID_HANDLE_VALUE, 
        NULL, 
        PAGE_READWRITE | SEC_COMMIT, 
        _memSize >> 32, 
        _memSize, 
        "DataMap");  
    if (!m_memObj)  return false;

    LPBYTE lpData = (LPBYTE)MapViewOfFile(m_memObj, FILE_MAP_WRITE, 0, 0, 0);  
    if (lpData == NULL)  
    {  
        CloseHandle(m_memObj);  
        return 0;  
    }  

    return true;
}

#endif
