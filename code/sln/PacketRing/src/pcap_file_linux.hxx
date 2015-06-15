#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

int CPCAPFile::setFileSize(u_int64 _fileSize)
{
    if (INVALID_HANDLE_VALUE == m_fileHandle) return -1;

    int z = truncate(m_fileName, _fileSize);
    LOG_LASTERROR_AND_RETURN();

    m_fileSize = _fileSize;
}

int CPCAPFile::createMapFile(CStdString& _fileName, u_int64 _fileSize)
{
    m_fileHandle = ::open(_fileName,
                     _fileSize > 0 ? O_CREAT | O_RDWR : O_RDONLY, 
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); 
    if (m_fileHandle == -1)
        LOG_LASTERROR_AND_RETURN();

    u_int32 low32Size = 0;
    if (_fileSize != 0) setFileSize(_fileSize);
    else
    {
        m_fileSize = 0;
        low32Size = getSize((u_int32*)&m_fileSize);
        m_fileSize <<= 32;
        m_fileSize += low32Size;
    }


    //lseek64(m_hFile, _fileSize, SEEK_SET);     

    m_buf = (byte*)mmap(NULL, 
                        m_fileSize,
                        _fileSize > 0 ? PROT_READ | PROT_WRITE : PROT_READ,
                        MAP_SHARED, 
                        m_fileHandle, 
                        0);
    if (m_buf == MAP_FAILED)
        LOG_LASTERROR_AND_RETURN();

    return 0;
}

int CPCAPFile::close()
{
    if (!isValid()) return -1;
    int z = munmap(m_buf, 1024);
    CloseHandle(m_fileHandle);

    return z;
}

int CPCAPFile::flush()
{
    if (!isValid())   return -1;

    return msync(m_buf, 1024, 0);
}
